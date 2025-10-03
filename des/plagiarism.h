#ifndef PLAGIARISM_H
#define PLAGIARISM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_DOCUMENTS 20
#define MAX_TOKENS 10000
#define MAX_TOKEN_LENGTH 100
#define MAX_FILENAME_LENGTH 256
#define MAX_STRING_LENGTH 100000
#define HASH_TABLE_SIZE 10007
#define KGRAM_MAX_LENGTH 10

// Data Structures
typedef struct TokenNode {
    char token[MAX_TOKEN_LENGTH];
    struct TokenNode* next;
} TokenNode;

typedef struct LinkedList {
    TokenNode* head;
    TokenNode* tail;
    int size;
} LinkedList;

typedef struct KGram {
    char gram[MAX_TOKEN_LENGTH * 10];
    int hash;
    struct KGram* next;
} KGram;

typedef struct HashSet {
    KGram** table;
    int size;
    int count;
} HashSet;

typedef struct Document {
    char filename[MAX_FILENAME_LENGTH];
    LinkedList* tokens;
    HashSet* kgrams;
    int token_count;
    int kgram_count;
} Document;

typedef struct SimilarityResult {
    char filename[MAX_FILENAME_LENGTH];
    double jaccard;
    double cosine;
    double containment;
    double dice;
    double overall;
    int matching_kgrams;
    char common_phrases[5][MAX_TOKEN_LENGTH * 10];
    int phrase_count;
} SimilarityResult;

// Function declarations
LinkedList* create_linked_list();
void list_add(LinkedList* list, const char* token);
void free_list(LinkedList* list);

HashSet* create_hash_set(int size);
unsigned int hash_function(const char* str);
void hash_set_add(HashSet* set, const char* kgram);
int hash_set_contains(HashSet* set, const char* kgram);
int hash_set_intersection_size(HashSet* set1, HashSet* set2);
int hash_set_union_size(HashSet* set1, HashSet* set2);
void free_hash_set(HashSet* set);

Document* create_document(const char* filename);
void preprocess_document(Document* doc, const char* text);
void generate_kgrams(Document* doc, int k);
void free_document(Document* doc);

// Similarity Algorithms
double jaccard_similarity(HashSet* set1, HashSet* set2);
double cosine_similarity(HashSet* set1, HashSet* set2);
double containment_similarity(HashSet* set1, HashSet* set2);
double dice_coefficient(HashSet* set1, HashSet* set2);

// String matching algorithms
void find_common_phrases(Document* target, Document* reference, 
                         char phrases[5][MAX_TOKEN_LENGTH * 10], int* phrase_count);

// Utility functions
void to_lowercase(char* str);
void remove_punctuation(char* str);
int is_stopword(const char* word);

#endif