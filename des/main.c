#include "plagiarism.h"

#define KGRAM_MAX_LENGTH 10

void print_usage() {
    printf("Advanced Plagiarism Checker\n");
    printf("Usage: ./plagiarism_checker\n");
    printf("Follow the interactive prompts to analyze documents.\n");
}

void print_results(SimilarityResult* results, int count, Document* target, int k) {
    printf("\n===RESULTS_START===\n");
    printf("{\n");
    printf("  \"target_stats\": {\n");
    printf("    \"filename\": \"%s\",\n", target->filename);
    printf("    \"tokens\": %d,\n", target->token_count);
    printf("    \"kgrams\": %d,\n", target->kgram_count);
    printf("    \"k_value\": %d\n", k);
    printf("  },\n");
    printf("  \"comparisons\": [\n");
    
    for (int i = 0; i < count; i++) {
        printf("    {\n");
        printf("      \"filename\": \"%s\",\n", results[i].filename);
        printf("      \"jaccard\": %.4f,\n", results[i].jaccard);
        printf("      \"cosine\": %.4f,\n", results[i].cosine);
        printf("      \"containment\": %.4f,\n", results[i].containment);
        printf("      \"dice\": %.4f,\n", results[i].dice);
        printf("      \"overall\": %.4f,\n", results[i].overall);
        printf("      \"matching_kgrams\": %d,\n", results[i].matching_kgrams);
        printf("      \"common_phrases\": [\n");
        
        for (int j = 0; j < results[i].phrase_count; j++) {
            printf("        \"%s\"%s\n", results[i].common_phrases[j], 
                   j < results[i].phrase_count - 1 ? "," : "");
        }
        printf("      ]\n");
        printf("    }%s\n", i < count - 1 ? "," : "");
    }
    
    printf("  ]\n");
    printf("}\n");
    printf("===RESULTS_END===\n");
}

int main() {
    Document* target = NULL;
    Document* references[MAX_DOCUMENTS];
    SimilarityResult results[MAX_DOCUMENTS];
    int ref_count = 0;
    int k = 3;
    
    printf("=== ADVANCED PLAGIARISM CHECKER ===\n");
    printf("Data Structures Used:\n");
    printf("- Linked Lists for token storage\n");
    printf("- Hash Sets for k-gram storage\n");
    printf("- Multiple similarity algorithms\n\n");
    
    // Get k-value
    printf("Enter k-value for k-grams (3-7 recommended): ");
    scanf("%d", &k);
    getchar();
    
    if (k < 2 || k > 10) {
        printf("Invalid k-value. Using default k=3.\n");
        k = 3;
    }
    
    // Read target document
    printf("\n=== TARGET DOCUMENT ===\n");
    printf("Enter target filename: ");
    char target_filename[MAX_FILENAME_LENGTH];
    fgets(target_filename, sizeof(target_filename), stdin);
    target_filename[strcspn(target_filename, "\n")] = 0;
    
    printf("Enter target text (end with empty line):\n");
    char target_text[MAX_STRING_LENGTH] = "";
    char line[1000];
    
    while (fgets(line, sizeof(line), stdin)) {
        if (strcmp(line, "\n") == 0) break;
        if (strlen(target_text) + strlen(line) < MAX_STRING_LENGTH - 1) {
            strcat(target_text, line);
        }
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
    scanf("%d", &ref_count);
    getchar();
    
    if (ref_count > MAX_DOCUMENTS) {
        printf("Too many references. Limiting to %d.\n", MAX_DOCUMENTS);
        ref_count = MAX_DOCUMENTS;
    }
    
    for (int i = 0; i < ref_count; i++) {
        printf("\nReference %d:\n", i + 1);
        printf("Enter filename: ");
        char ref_filename[MAX_FILENAME_LENGTH];
        fgets(ref_filename, sizeof(ref_filename), stdin);
        ref_filename[strcspn(ref_filename, "\n")] = 0;
        
        printf("Enter content (end with empty line):\n");
        char ref_text[MAX_STRING_LENGTH] = "";
        
        while (fgets(line, sizeof(line), stdin)) {
            if (strcmp(line, "\n") == 0) break;
            if (strlen(ref_text) + strlen(line) < MAX_STRING_LENGTH - 1) {
                strcat(ref_text, line);
            }
        }
        
        references[i] = create_document(ref_filename);
        preprocess_document(references[i], ref_text);
        generate_kgrams(references[i], k);
        
        printf("Reference processed: %d tokens, %d k-grams\n", 
               references[i]->token_count, references[i]->kgram_count);
    }
    
    // Perform comparisons
    printf("\n=== ANALYZING SIMILARITY ===\n");
    for (int i = 0; i < ref_count; i++) {
        strcpy(results[i].filename, references[i]->filename);
        
        // Calculate all similarity measures
        results[i].jaccard = jaccard_similarity(target->kgrams, references[i]->kgrams);
        results[i].cosine = cosine_similarity(target->kgrams, references[i]->kgrams);
        results[i].containment = containment_similarity(target->kgrams, references[i]->kgrams);
        results[i].dice = dice_coefficient(target->kgrams, references[i]->kgrams);
        
        // Calculate overall similarity (weighted average)
        results[i].overall = (results[i].jaccard + results[i].cosine + 
                            results[i].containment + results[i].dice) / 4.0;
        
        // Count matching k-grams
        results[i].matching_kgrams = hash_set_intersection_size(target->kgrams, 
                                                              references[i]->kgrams);
        
        // Find common phrases
        find_common_phrases(target, references[i], 
                          results[i].common_phrases, &results[i].phrase_count);
        
        printf("Compared with %s: %.1f%% similar\n", 
               references[i]->filename, results[i].overall * 100);
    }
    
    // Print results in JSON format
    print_results(results, ref_count, target, k);
    
    // Cleanup
    free_document(target);
    for (int i = 0; i < ref_count; i++) {
        free_document(references[i]);
    }
    
    return 0;
}