#include "plagiarism.h"

void write_json_results(SimilarityResult* results, int count, Document* target, int k, const char* output_file) {
    FILE* fp = fopen(output_file, "w");
    if (fp == NULL) {
        printf("Error: Cannot create output file\n");
        return;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"target_stats\": {\n");
    fprintf(fp, "    \"filename\": \"%s\",\n", target->filename);
    fprintf(fp, "    \"tokens\": %d,\n", target->token_count);
    fprintf(fp, "    \"kgrams\": %d,\n", target->kgram_count);
    fprintf(fp, "    \"k_value\": %d\n", k);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"comparisons\": [\n");
    
    for (int i = 0; i < count; i++) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"filename\": \"%s\",\n", results[i].filename);
        fprintf(fp, "      \"jaccard\": %.4f,\n", results[i].jaccard);
        fprintf(fp, "      \"cosine\": %.4f,\n", results[i].cosine);
        fprintf(fp, "      \"containment\": %.4f,\n", results[i].containment);
        fprintf(fp, "      \"dice\": %.4f,\n", results[i].dice);
        fprintf(fp, "      \"overall\": %.4f,\n", results[i].overall);
        fprintf(fp, "      \"matching_kgrams\": %d,\n", results[i].matching_kgrams);
        fprintf(fp, "      \"common_phrases\": [\n");
        
        for (int j = 0; j < results[i].phrase_count; j++) {
            // Escape quotes in phrases for JSON
            char escaped_phrase[MAX_TOKEN_LENGTH * 10];
            strcpy(escaped_phrase, results[i].common_phrases[j]);
            // Simple escape - replace " with \"
            for (char* p = escaped_phrase; *p; p++) {
                if (*p == '"') {
                    memmove(p + 1, p, strlen(p) + 1);
                    *p = '\\';
                    p++;
                }
            }
            
            fprintf(fp, "        \"%s\"%s\n", escaped_phrase, 
                   j < results[i].phrase_count - 1 ? "," : "");
        }
        fprintf(fp, "      ]\n");
        fprintf(fp, "    }%s\n", i < count - 1 ? "," : "");
    }
    
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

int main(int argc, char* argv[]) {
    Document* target = NULL;
    Document* references[MAX_DOCUMENTS];
    SimilarityResult results[MAX_DOCUMENTS];
    int ref_count = 0;
    int k = 3;
    char output_file[256] = "results.json";
    
    // Parse command line arguments
    if (argc < 4) {
        printf("Usage: %s <k_value> <target_file> <ref_file1> [ref_file2 ...] [output_file]\n", argv[0]);
        printf("Using interactive mode...\n\n");
    } else {
        // Command line mode
        k = atoi(argv[1]);
        if (k < 2 || k > 10) k = 3;
        
        // Read target file
        FILE* target_fp = fopen(argv[2], "r");
        if (target_fp == NULL) {
            printf("Error: Cannot open target file %s\n", argv[2]);
            return 1;
        }
        
        char target_text[MAX_STRING_LENGTH] = "";
        char line[1000];
        while (fgets(line, sizeof(line), target_fp)) {
            if (strlen(target_text) + strlen(line) < MAX_STRING_LENGTH - 1) {
                strcat(target_text, line);
            }
        }
        fclose(target_fp);
        
        target = create_document(argv[2]);
        preprocess_document(target, target_text);
        generate_kgrams(target, k);
        
        // Read reference files
        ref_count = argc - 3;
        if (argc > 3 && strstr(argv[argc-1], ".json")) {
            strcpy(output_file, argv[argc-1]);
            ref_count--; // Last argument is output file
        }
        
        for (int i = 0; i < ref_count; i++) {
            FILE* ref_fp = fopen(argv[3 + i], "r");
            if (ref_fp == NULL) {
                printf("Warning: Cannot open reference file %s, skipping\n", argv[3 + i]);
                references[i] = NULL;
                continue;
            }
            
            char ref_text[MAX_STRING_LENGTH] = "";
            while (fgets(line, sizeof(line), ref_fp)) {
                if (strlen(ref_text) + strlen(line) < MAX_STRING_LENGTH - 1) {
                    strcat(ref_text, line);
                }
            }
            fclose(ref_fp);
            
            references[i] = create_document(argv[3 + i]);
            preprocess_document(references[i], ref_text);
            generate_kgrams(references[i], k);
        }
        
        goto analyze; // Skip interactive mode
    }
    
    // Interactive mode
    printf("=== ADVANCED PLAGIARISM CHECKER ===\n");
    
    // Get k-value
    printf("Enter k-value for k-grams (3-7 recommended): ");
    if (scanf("%d", &k) != 1) {
        printf("Invalid input. Using default k=3.\n");
        k = 3;
    }
    getchar();
    
    if (k < 2 || k > 10) {
        printf("Invalid k-value. Using default k=3.\n");
        k = 3;
    }
    
    // Read target document
    printf("\n=== TARGET DOCUMENT ===\n");
    printf("Enter target filename: ");
    char target_filename[MAX_FILENAME_LENGTH];
    if (fgets(target_filename, sizeof(target_filename), stdin) == NULL) {
        printf("Error reading filename.\n");
        return 1;
    }
    target_filename[strcspn(target_filename, "\n")] = 0;
    
    printf("Enter target text (end with empty line):\n");
    char target_text[MAX_STRING_LENGTH] = "";
    char line[1000];
    
    while (fgets(line, sizeof(line), stdin)) {
        if (strcmp(line, "\n") == 0) break;
        if (strlen(target_text) + strlen(line) < MAX_STRING_LENGTH - 1) {
            strcat(target_text, line);
        } else {
            printf("Warning: Target text too long, truncating.\n");
            break;
        }
    }
    
    if (strlen(target_text) == 0) {
        printf("Error: No target text provided.\n");
        return 1;
    }
    
    // Process target document
    target = create_document(target_filename);
    preprocess_document(target, target_text);
    generate_kgrams(target, k);
    
    printf("Target processed: %d tokens, %d k-grams\n", 
           target->token_count, target->kgram_count);
    
    // Read reference documents
    printf("\n=== REFERENCE DOCUMENTS ===\n");
    printf("Enter number of reference documents: ");
    if (scanf("%d", &ref_count) != 1) {
        printf("Invalid input. Using 0 references.\n");
        ref_count = 0;
    }
    getchar();
    
    if (ref_count > MAX_DOCUMENTS) {
        printf("Too many references. Limiting to %d.\n", MAX_DOCUMENTS);
        ref_count = MAX_DOCUMENTS;
    }
    
    for (int i = 0; i < ref_count; i++) {
        printf("\nReference %d:\n", i + 1);
        printf("Enter filename: ");
        char ref_filename[MAX_FILENAME_LENGTH];
        if (fgets(ref_filename, sizeof(ref_filename), stdin) == NULL) {
            printf("Error reading filename.\n");
            continue;
        }
        ref_filename[strcspn(ref_filename, "\n")] = 0;
        
        printf("Enter content (end with empty line):\n");
        char ref_text[MAX_STRING_LENGTH] = "";
        
        while (fgets(line, sizeof(line), stdin)) {
            if (strcmp(line, "\n") == 0) break;
            if (strlen(ref_text) + strlen(line) < MAX_STRING_LENGTH - 1) {
                strcat(ref_text, line);
            } else {
                printf("Warning: Reference text too long, truncating.\n");
                break;
            }
        }
        
        if (strlen(ref_text) == 0) {
            printf("Warning: No content for reference %s, skipping.\n", ref_filename);
            references[i] = NULL;
            continue;
        }
        
        references[i] = create_document(ref_filename);
        preprocess_document(references[i], ref_text);
        generate_kgrams(references[i], k);
        
        printf("Reference processed: %d tokens, %d k-grams\n", 
               references[i]->token_count, references[i]->kgram_count);
    }

analyze:
    // Perform comparisons
    printf("\n=== ANALYZING SIMILARITY ===\n");
    int valid_comparisons = 0;
    for (int i = 0; i < ref_count; i++) {
        if (references[i] == NULL) continue;
        
        strcpy(results[valid_comparisons].filename, references[i]->filename);
        
        // Calculate all similarity measures
        results[valid_comparisons].jaccard = jaccard_similarity(target->kgrams, references[i]->kgrams);
        results[valid_comparisons].cosine = cosine_similarity(target->kgrams, references[i]->kgrams);
        results[valid_comparisons].containment = containment_similarity(target->kgrams, references[i]->kgrams);
        results[valid_comparisons].dice = dice_coefficient(target->kgrams, references[i]->kgrams);
        
        // Calculate overall similarity (weighted average)
        results[valid_comparisons].overall = (results[valid_comparisons].jaccard + 
                                            results[valid_comparisons].cosine + 
                                            results[valid_comparisons].containment + 
                                            results[valid_comparisons].dice) / 4.0;
        
        // Count matching k-grams
        results[valid_comparisons].matching_kgrams = hash_set_intersection_size(target->kgrams, 
                                                                              references[i]->kgrams);
        
        // Find common phrases
        find_common_phrases(target, references[i], 
                          results[valid_comparisons].common_phrases, 
                          &results[valid_comparisons].phrase_count);
        
        printf("Compared with %s: %.1f%% similar\n", 
               references[i]->filename, results[valid_comparisons].overall * 100);
        valid_comparisons++;
    }
    
    // Write results to JSON file for frontend
    write_json_results(results, valid_comparisons, target, k, output_file);
    printf("Results written to %s\n", output_file);
    
    // Cleanup
    if (target != NULL) {
        free_document(target);
    }
    for (int i = 0; i < ref_count; i++) {
        if (references[i] != NULL) {
            free_document(references[i]);
        }
    }
    
    return 0;
}