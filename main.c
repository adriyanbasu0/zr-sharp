#include "compiler.h"
#include <stdarg.h> // For va_list, va_start, va_end
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strcmp, strcpy, strcat
#include <limits.h> // For PATH_MAX (maybe) or use a defined MAX_MODULE_PATH_LEN
#include <unistd.h> // For getcwd (maybe, for resolving main_script_dir if path is relative)
#include <sys/stat.h> // For checking file existence
#include <errno.h>    // For errno

#include "debug.h" 
#include "compiler.h" // For LoadedModulesRegistry, MAX_MODULE_PATH_LEN

// Global registry for loaded modules
static LoadedModulesRegistry loaded_modules_registry;

// Initialize the loaded modules registry
void init_loaded_modules_registry() {
    loaded_modules_registry.count = 0;
    for (int i = 0; i < MAX_LOADED_MODULES; ++i) {
        loaded_modules_registry.paths[i][0] = '\0';
    }
}

// Helper function to check if a file exists
static bool file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode));
}

// Helper function to extract directory part from a path
// Puts the directory part into dir_buffer.
// Returns true on success, false if path is too long or no directory part.
static bool get_directory_part(const char* path, char* dir_buffer, size_t buffer_size) {
    const char* last_slash = strrchr(path, '/');
    if (last_slash == NULL) { // No slash, so no directory part (or it's just a filename in current dir)
        // Consider it as current directory "."
        if (buffer_size > 1) {
            strcpy(dir_buffer, ".");
            return true;
        }
        return false; // Buffer too small
    }
    size_t dir_len = last_slash - path;
    if (dir_len + 1 > buffer_size) {
        return false; // Buffer too small
    }
    if (dir_len == 0) { // Path starts with '/', e.g., "/file.zr"
         if (buffer_size > 1) {
            strcpy(dir_buffer, "/"); // Root directory
            return true;
        }
        return false;
    }
    strncpy(dir_buffer, path, dir_len);
    dir_buffer[dir_len] = '\0';
    return true;
}

// Helper function to build a path safely
// Concatenates base and component into out_buffer.
// Ensures not to overflow out_buffer.
static bool build_path(const char* base, const char* component, char* out_buffer, size_t buffer_size) {
    if (base == NULL || component == NULL || out_buffer == NULL) return false;

    // If component is already an absolute path, just use it (simplified)
    if (component[0] == '/') {
        if (strlen(component) + 1 > buffer_size) return false;
        strcpy(out_buffer, component);
        return true;
    }

    size_t base_len = strlen(base);
    size_t component_len = strlen(component);

    // Check for overflow: base_len + (optional '/') + component_len + null_terminator
    if (base_len + (base_len > 0 && base[base_len - 1] != '/' ? 1 : 0) + component_len + 1 > buffer_size) {
        return false;
    }

    strcpy(out_buffer, base);
    if (base_len > 0 && out_buffer[base_len - 1] != '/' && component[0] != '\0') {
        strcat(out_buffer, "/");
    }
    strcat(out_buffer, component);
    return true;
}


// Resolve module path:
// 1. Appends ".zr" if not present.
// 2. Tries path relative to current_file_dir.
// 3. Tries path relative to main_script_dir/files/
// Returns an allocated string with a canonicalized absolute path if found, otherwise NULL.
// Caller must free the returned string.
static char* resolve_module_path(const char* requested_path_in, const char* current_file_dir, const char* main_script_dir) {
    char module_name_or_rel_path[MAX_MODULE_PATH_LEN];
    strncpy(module_name_or_rel_path, requested_path_in, MAX_MODULE_PATH_LEN - 1);
    module_name_or_rel_path[MAX_MODULE_PATH_LEN - 1] = '\0';

    char requested_file[MAX_MODULE_PATH_LEN];
    if (strlen(module_name_or_rel_path) + 4 > MAX_MODULE_PATH_LEN) {
         error("Error: Module path '%s' is too long to append .zr.", module_name_or_rel_path);
         return NULL;
    }
    snprintf(requested_file, MAX_MODULE_PATH_LEN, "%s.zr", module_name_or_rel_path);

    char candidate_path_buffer[MAX_MODULE_PATH_LEN]; // Buffer for path construction
    char* real_path_result = NULL; // For realpath result
    char* final_resolved_path = NULL; // To store the strdup'd version of real_path_result

    // Function to try resolving a candidate
    char* try_resolve(const char* candidate) {
        if (file_exists(candidate)) {
            char* rp = realpath(candidate, NULL);
            if (rp) {
                LOG_DEBUG("realpath for '%s' -> '%s'", candidate, rp);
                return rp; // Caller will strdup and free rp
            } else {
                // realpath can fail if path components don't exist, or perms, etc.
                // Since file_exists passed, this might be unusual, but good to log.
                LOG_WARN("realpath failed for existing file '%s': %s", candidate, strerror(errno));
                // Fallback to strdup(candidate) if realpath fails but file exists,
                // though this means we lose canonicalization benefits.
                // For stricter canonicalization, one might return NULL here.
                // For now, let's be a bit lenient if realpath fails on an existing file.
                return strdup(candidate);
            }
        }
        return NULL;
    }

    // 1. Try relative to current_file_dir
    if (current_file_dir) {
        if (build_path(current_file_dir, requested_file, candidate_path_buffer, MAX_MODULE_PATH_LEN)) {
            LOG_DEBUG("Resolve try 1 (relative to %s): %s", current_file_dir, candidate_path_buffer);
            real_path_result = try_resolve(candidate_path_buffer);
            if (real_path_result) goto found;
        }
    }

    // 2. Try relative to main_script_dir/files/
    if (main_script_dir) {
        char files_dir_base[MAX_MODULE_PATH_LEN];
        if (build_path(main_script_dir, "files", files_dir_base, MAX_MODULE_PATH_LEN)) {
            if (build_path(files_dir_base, requested_file, candidate_path_buffer, MAX_MODULE_PATH_LEN)) {
                LOG_DEBUG("Resolve try 2 (main_script_dir/files): %s", candidate_path_buffer);
                real_path_result = try_resolve(candidate_path_buffer);
                if (real_path_result) goto found;
            }
        }
    }

    // 3. If requested_path_in itself was an absolute path
    if (requested_path_in[0] == '/') {
        // requested_file here is already absolute + .zr
        LOG_DEBUG("Resolve try 3 (absolute path given): %s", requested_file);
        real_path_result = try_resolve(requested_file);
        if (real_path_result) goto found;
    }

found:
    if (real_path_result) {
        final_resolved_path = strdup(real_path_result);
        if (!final_resolved_path) {
            error("Memory allocation failed for final resolved path.");
            // real_path_result must be freed if strdup fails and we are about to exit or return NULL
            free(real_path_result);
            return NULL; // Or exit via error()
        }
        free(real_path_result); // Free the string allocated by realpath OR the strdup from try_resolve's fallback
        return final_resolved_path;
    }

    LOG_DEBUG("Module '%s' (as '%s') not found with current_file_dir='%s', main_script_dir='%s'",
            requested_path_in, requested_file, current_file_dir ? current_file_dir : "NULL", main_script_dir ? main_script_dir : "NULL");
    return NULL;
}

// Check if a module is already loaded or currently loading. If not, register it.
// Returns true if module can be loaded, false if it's a duplicate (error).
// Uses the global loaded_modules_registry.
static bool check_and_register_module(const char* canonical_path) {
    if (canonical_path == NULL) return false; // Should not happen

    for (int i = 0; i < loaded_modules_registry.count; ++i) {
        if (strcmp(loaded_modules_registry.paths[i], canonical_path) == 0) {
            // Module already loaded or in the process of loading (as per Scenario 2 strict check)
            error("Error: Module '%s' is already loaded or causes a circular dependency.", canonical_path);
            return false;
        }
    }

    if (loaded_modules_registry.count >= MAX_LOADED_MODULES) {
        error("Error: Maximum number of loaded modules (%d) exceeded.", MAX_LOADED_MODULES);
        return false;
    }

    strncpy(loaded_modules_registry.paths[loaded_modules_registry.count], canonical_path, MAX_MODULE_PATH_LEN -1);
    loaded_modules_registry.paths[loaded_modules_registry.count][MAX_MODULE_PATH_LEN -1] = '\0';
    loaded_modules_registry.count++;

    return true;
}


// Function to report errors and exit
void error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: "); // Prefix with "Error: "
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n"); // Add a newline for better formatting
    va_end(args);
    // Note: exit(1) should only be called from the top-level main or if recovery is impossible.
    // In recursive loadin, we might want to propagate errors differently.
    // For now, keeping exit(1) as it's a fatal error handler.
    exit(1);
}

// Forward declaration for recursive processing
static void process_source_code(char* source_code, const char* source_filepath, const char* main_script_dir);


// Processes a single source file: lexes, parses, handles loadin directives, and interprets other statements.
static void process_source_code(char* source_code, const char* source_filepath, const char* main_script_dir) {
    if (source_code == NULL || source_filepath == NULL) {
        error("Internal Error: process_source_code called with NULL source or filepath.");
        return;
    }
    LOG_DEBUG("Processing source file: %s", source_filepath);

    Lexer* lexer = init_lexer(source_code);
    Parser* parser = init_parser(lexer);
    ASTNode* program_ast = parse_program(parser); // This is a NODE_BLOCK

    if (program_ast == NULL || program_ast->type != NODE_BLOCK) {
        error("Error: Failed to parse program: %s. Expected top-level NODE_BLOCK.", source_filepath);
        // Cleanup for this level of processing
        if (parser) free_parser(parser);
        if (lexer) free_lexer(lexer);
        // program_ast might be NULL or an invalid node, free_ast should handle NULL.
        if (program_ast) free_ast(program_ast);
        return; // Or propagate error
    }

    // Create a temporary block for non-loadin statements of the current file
    ASTNode* current_file_code_block = create_node(NODE_BLOCK);
    current_file_code_block->statements = safe_malloc(sizeof(ASTNode*) * program_ast->statement_count); // Max possible needed
    current_file_code_block->statement_count = 0;


    // First pass: handle NODE_LOADIN directives and collect other statements
    for (int i = 0; i < program_ast->statement_count; ++i) {
        ASTNode* stmt = program_ast->statements[i];
        if (stmt == NULL) continue;

        if (stmt->type == NODE_LOADIN) {
            if (stmt->value.string_val == NULL) {
                error("Internal Error: NODE_LOADIN found with NULL path in %s.", source_filepath);
                continue;
            }
            LOG_DEBUG("Found loadin directive for: %s in %s", stmt->value.string_val, source_filepath);

            char current_dir_buffer[MAX_MODULE_PATH_LEN];
            if (!get_directory_part(source_filepath, current_dir_buffer, MAX_MODULE_PATH_LEN)) {
                error("Error: Could not determine directory for current file %s to resolve loadin.", source_filepath);
                continue; // Or propagate error
            }

            char* resolved_module_path = resolve_module_path(stmt->value.string_val, current_dir_buffer, main_script_dir);
            if (resolved_module_path == NULL) {
                error("Error: Failed to resolve module '%s' requested in %s.", stmt->value.string_val, source_filepath);
                // Continue to allow other statements to be processed or error out? For now, error out.
                // Freeing collected statements and program_ast before exit
                free_ast(current_file_code_block);
                // program_ast statements are moved or will be freed with program_ast.
                // Need to be careful here: stmt is part of program_ast.
                // If error() exits, memory will be leaked.
                // This error path needs careful thought for resource cleanup in recursive calls.
                // For now, relying on error() to exit.
                 error("Exiting due to module resolution failure."); // This will call exit(1)
            }

            if (check_and_register_module(resolved_module_path)) {
                LOG_INFO("Loading module: %s", resolved_module_path);
                FILE* module_file = fopen(resolved_module_path, "r");
                if (!module_file) {
                    error("Error: Could not open module file '%s'.", resolved_module_path);
                    safe_free(resolved_module_path); // Free path if file open fails
                    // Similar error path cleanup concerns as above.
                    error("Exiting due to file open failure.");
                }

                fseek(module_file, 0, SEEK_END);
                long module_size_long = ftell(module_file);
                fseek(module_file, 0, SEEK_SET);

                if (module_size_long < 0) { // Error check for ftell
                    error("Error: Could not determine size of module file '%s'.", resolved_module_path);
                }
                size_t module_size = (size_t)module_size_long; // Cast to size_t

                char* module_source = safe_malloc(module_size + 1);
                if (fread(module_source, 1, module_size, module_file) != module_size) {
                     error("Error reading module file '%s'", resolved_module_path);
                }
                module_source[module_size] = '\0';
                fclose(module_file);

                process_source_code(module_source, resolved_module_path, main_script_dir); // Recursive call

                safe_free(module_source);
            }
            safe_free(resolved_module_path); // Free after use
            // The NODE_LOADIN ast node itself (stmt) will be freed when program_ast is freed.
        } else {
            // Add non-NODE_LOADIN statements to the current_file_code_block
            // We are "moving" the statement pointer.
            current_file_code_block->statements[current_file_code_block->statement_count++] = stmt;
            program_ast->statements[i] = NULL; // Nullify in original AST to prevent double free
        }
    }

    // After all loadins are processed, interpret the collected statements for the current file
    if (current_file_code_block->statement_count > 0) {
        LOG_DEBUG("Interpreting collected code for: %s", source_filepath);
        interpret(current_file_code_block); // interpret will free its AST content
    } else {
        free_ast(current_file_code_block); // Free if empty (no statements moved)
    }

    // Free the original program AST shell (statements were moved or are NODE_LOADINs which are conceptually handled)
    // Nullify statements array in program_ast before freeing, as its contents were moved or are handled.
    if (program_ast) {
        // for(int i=0; i < program_ast->statement_count; ++i) program_ast->statements[i] = NULL; // Ensure children aren't re-freed
        // The above loop is problematic if some stmts were NODE_LOADIN and not moved.
        // free_ast should handle freeing children of program_ast correctly if they weren't nulled out.
        // If a child was moved to current_file_code_block, its pointer in program_ast->statements was set to NULL.
        // So, free_ast(program_ast) should only free the remaining NODE_LOADINs and the block itself.
        free_ast(program_ast);
    }

    // Free lexer and parser for the current file
    if (parser) free_parser(parser);
    if (lexer) free_lexer(lexer);
     LOG_DEBUG("Finished processing source file: %s", source_filepath);
}


int main(int argc, char* argv[]) {
    set_debug_level(DEBUG_LEVEL_DEBUG);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    init_loaded_modules_registry(); // Initialize the global registry

    char* initial_filepath_arg = argv[1];
    // char main_script_abs_path[MAX_MODULE_PATH_LEN]; // Unused variable removed
    char main_script_dir[MAX_MODULE_PATH_LEN];

    // Attempt to get absolute path for the main script.
    // This is a simplified approach. Realpath or more robust library needed for production.
    // For now, if it's relative, it's relative to CWD.
    // If initial_filepath_arg is already absolute, strncpy works.
    // If it's relative, this doesn't make it absolute based on CWD yet.
    // Let's assume for now that argv[1] can be used "as-is" for fopen,
    // and get_directory_part will handle it (if it's just "file.zr", dir is ".").

    // A more robust way to get main_script_dir if initial_filepath_arg can be relative:
    char initial_file_fullpath[PATH_MAX];
    if (initial_filepath_arg[0] == '/') { // Already absolute
        strncpy(initial_file_fullpath, initial_filepath_arg, PATH_MAX -1);
    } else { // Relative, prepend CWD
        if (getcwd(initial_file_fullpath, sizeof(initial_file_fullpath)) != NULL) {
            strncat(initial_file_fullpath, "/", PATH_MAX - strlen(initial_file_fullpath) -1);
            strncat(initial_file_fullpath, initial_filepath_arg, PATH_MAX - strlen(initial_file_fullpath) -1);
        } else {
            error("Error: Could not get current working directory.");
        }
    }
    initial_file_fullpath[PATH_MAX-1] = '\0'; // Ensure null termination


    if (!get_directory_part(initial_file_fullpath, main_script_dir, MAX_MODULE_PATH_LEN)) {
        error("Error: Could not determine directory for main script file '%s'.", initial_file_fullpath);
        // If main_script_dir is critical and get_directory_part fails (e.g. if initial_file_fullpath was just "file.zr")
        // We can default main_script_dir to "." if appropriate or error out.
        // Given get_directory_part returns "." for "file.zr", this path might not be hit often unless buffer is too small.
    }
    
    LOG_INFO("Main script directory set to: %s", main_script_dir);
    LOG_INFO("Main script full path (attempted): %s", initial_file_fullpath);


    FILE* file = fopen(initial_filepath_arg, "r"); // Use original argv[1] for fopen
    if (!file) {
        error("Error: Could not open file '%s'\n", initial_filepath_arg);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size_long = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size_long < 0) { // Error check for ftell
        error("Error: Could not determine size of main file '%s'.", initial_filepath_arg);
    }
    size_t size = (size_t)size_long; // Cast to size_t

    char* initial_source_code = safe_malloc(size + 1);
    size_t read_size = fread(initial_source_code, 1, size, file);
    if (read_size != size) {
         error("Error reading main file '%s'", initial_filepath_arg);
    }
    initial_source_code[read_size] = '\0';
    fclose(file);

    // Use initial_file_fullpath for the source_filepath context
    process_source_code(initial_source_code, initial_file_fullpath, main_script_dir);

    safe_free(initial_source_code);
    free_interpreter_memory(); // Cleans up global symbol table etc.

    LOG_INFO("Execution finished.");
    return 0;
}