#include "plagiarism.h"

// Linked List Implementation
LinkedList* create_linked_list() {
    LinkedList* list = (LinkedList*)malloc(sizeof(LinkedList));
    list->head = list->tail = NULL;
    list->size = 0;
    return list;
}

void list_add(LinkedList* list, const char* token) {
    TokenNode* newNode = (TokenNode*)malloc(sizeof(TokenNode));
    strncpy(newNode->token, token, MAX_TOKEN_LENGTH - 1);
    newNode->token[MAX_TOKEN_LENGTH - 1] = '\0';
    newNode->next = NULL;
    
    if (list->tail == NULL) {
        list->head = list->tail = newNode;
    } else {
        list->tail->next = newNode;
        list->tail = newNode;
    }
    list->size++;
}

void free_list(LinkedList* list) {
    TokenNode* current = list->head;
    while (current != NULL) {
        TokenNode* next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

// Hash Set Implementation for K-grams
HashSet* create_hash_set(int size) {
    HashSet* set = (HashSet*)malloc(sizeof(HashSet));
    set->size = size;
    set->count = 0;
    set->table = (KGram**)calloc(size, sizeof(KGram*));
    return set;
}

unsigned int hash_function(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}

void hash_set_add(HashSet* set, const char* kgram) {
    unsigned int index = hash_function(kgram);
    
    // Check if already exists
    KGram* current = set->table[index];
    while (current != NULL) {
        if (strcmp(current->gram, kgram) == 0) {
            return; // Already exists
        }
        current = current->next;
    }
    
    // Add new k-gram
    KGram* newKGram = (KGram*)malloc(sizeof(KGram));
    strncpy(newKGram->gram, kgram, MAX_TOKEN_LENGTH * 10 - 1);
    newKGram->gram[MAX_TOKEN_LENGTH * 10 - 1] = '\0';
    newKGram->hash = index;
    newKGram->next = set->table[index];
    set->table[index] = newKGram;
    set->count++;
}

int hash_set_contains(HashSet* set, const char* kgram) {
    unsigned int index = hash_function(kgram);
    KGram* current = set->table[index];
    
    while (current != NULL) {
        if (strcmp(current->gram, kgram) == 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

int hash_set_intersection_size(HashSet* set1, HashSet* set2) {
    int intersection = 0;
    
    for (int i = 0; i < set1->size; i++) {
        KGram* current = set1->table[i];
        while (current != NULL) {
            if (hash_set_contains(set2, current->gram)) {
                intersection++;
            }
            current = current->next;
        }
    }
    return intersection;
}

int hash_set_union_size(HashSet* set1, HashSet* set2) {
    return set1->count + set2->count - hash_set_intersection_size(set1, set2);
}

void free_hash_set(HashSet* set) {
    for (int i = 0; i < set->size; i++) {
        KGram* current = set->table[i];
        while (current != NULL) {
            KGram* next = current->next;
            free(current);
            current = next;
        }
    }
    free(set->table);
    free(set);
}

// Document Management
Document* create_document(const char* filename) {
    Document* doc = (Document*)malloc(sizeof(Document));
    strncpy(doc->filename, filename, MAX_FILENAME_LENGTH - 1);
    doc->filename[MAX_FILENAME_LENGTH - 1] = '\0';
    doc->tokens = create_linked_list();
    doc->kgrams = create_hash_set(HASH_TABLE_SIZE);
    doc->token_count = 0;
    doc->kgram_count = 0;
    return doc;
}

void preprocess_document(Document* doc, const char* text) {
    char buffer[MAX_TOKEN_LENGTH];
    int buffer_index = 0;
    char processed_text[MAX_STRING_LENGTH];
    
    // Preprocess the entire text
    strncpy(processed_text, text, MAX_STRING_LENGTH - 1);
    processed_text[MAX_STRING_LENGTH - 1] = '\0';
    to_lowercase(processed_text);
    remove_punctuation(processed_text);
    
    // Tokenize
    for (int i = 0; processed_text[i] && doc->token_count < MAX_TOKENS; i++) {
        if (isspace(processed_text[i])) {
            if (buffer_index > 0) {
                buffer[buffer_index] = '\0';
                if (!is_stopword(buffer)) {
                    list_add(doc->tokens, buffer);
                    doc->token_count++;
                }
                buffer_index = 0;
            }
        } else {
            if (buffer_index < MAX_TOKEN_LENGTH - 1) {
                buffer[buffer_index++] = processed_text[i];
            }
        }
    }
    
    // Add last token
    if (buffer_index > 0 && doc->token_count < MAX_TOKENS) {
        buffer[buffer_index] = '\0';
        if (!is_stopword(buffer)) {
            list_add(doc->tokens, buffer);
            doc->token_count++;
        }
    }
}

void generate_kgrams(Document* doc, int k) {
    if (doc->token_count < k) return;
    
    // Create an array to store current window of tokens
    TokenNode* current = doc->tokens->head;
    
    // Use a simple approach without fixed array size
    while (current != NULL) {
        // Check if we have enough tokens ahead for a k-gram
        TokenNode* temp = current;
        int can_form_kgram = 1;
        
        // Verify we have k tokens ahead
        for (int i = 0; i < k; i++) {
            if (temp == NULL) {
                can_form_kgram = 0;
                break;
            }
            temp = temp->next;
        }
        
        if (!can_form_kgram) break;
        
        // Build the k-gram
        char kgram[MAX_TOKEN_LENGTH * 10] = "";
        temp = current;
        for (int i = 0; i < k; i++) {
            if (i > 0) {
                strcat(kgram, " ");
            }
            strcat(kgram, temp->token);
            temp = temp->next;
        }
        
        hash_set_add(doc->kgrams, kgram);
        doc->kgram_count++;
        
        current = current->next;
    }
}

void free_document(Document* doc) {
    free_list(doc->tokens);
    free_hash_set(doc->kgrams);
    free(doc);
}

// Similarity Algorithms
double jaccard_similarity(HashSet* set1, HashSet* set2) {
    if (set1->count == 0 || set2->count == 0) return 0.0;
    
    int intersection = hash_set_intersection_size(set1, set2);
    int union_size = hash_set_union_size(set1, set2);
    
    return union_size > 0 ? (double)intersection / union_size : 0.0;
}

double cosine_similarity(HashSet* set1, HashSet* set2) {
    if (set1->count == 0 || set2->count == 0) return 0.0;
    
    int intersection = hash_set_intersection_size(set1, set2);
    double magnitude = sqrt(set1->count) * sqrt(set2->count);
    
    return magnitude > 0 ? intersection / magnitude : 0.0;
}

double containment_similarity(HashSet* set1, HashSet* set2) {
    if (set1->count == 0) return 0.0;
    
    int intersection = hash_set_intersection_size(set1, set2);
    return (double)intersection / set1->count;
}

double dice_coefficient(HashSet* set1, HashSet* set2) {
    if (set1->count == 0 || set2->count == 0) return 0.0;
    
    int intersection = hash_set_intersection_size(set1, set2);
    return (2.0 * intersection) / (set1->count + set2->count);
}

// Utility Functions
void to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void remove_punctuation(char* str) {
    int j = 0;
    for (int i = 0; str[i]; i++) {
        if (isalpha(str[i]) || str[i] == '\'' || isspace(str[i])) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
    
    // Collapse multiple spaces
    j = 0;
    int space_flag = 0;
    for (int i = 0; str[i]; i++) {
        if (isspace(str[i])) {
            if (!space_flag) {
                str[j++] = ' ';
                space_flag = 1;
            }
        } else {
            str[j++] = str[i];
            space_flag = 0;
        }
    }
    str[j] = '\0';
}

int is_stopword(const char* word) {
    // Common English stopwords
    const char* stopwords[] = {
        "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for", 
        "of", "with", "by", "as", "is", "was", "were", "be", "been", "have",
        "has", "had", "do", "does", "did", "will", "would", "could", "should",
        "may", "might", "must", "can", "i", "you", "he", "she", "it", "we",
        "they", "me", "him", "her", "us", "them", "this", "that", "these",
        "those", "my", "your", "his", "its", "our", "their", "am", "are"
    };
    
    int num_stopwords = sizeof(stopwords) / sizeof(stopwords[0]);
    for (int i = 0; i < num_stopwords; i++) {
        if (strcmp(word, stopwords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Find common phrases using simple sliding window
void find_common_phrases(Document* target, Document* reference, 
                         char phrases[5][MAX_TOKEN_LENGTH * 10], int* phrase_count) {
    *phrase_count = 0;
    
    // Convert documents to text for phrase matching
    char target_text[MAX_STRING_LENGTH] = "";
    char ref_text[MAX_STRING_LENGTH] = "";
    
    TokenNode* current = target->tokens->head;
    while (current != NULL && strlen(target_text) < MAX_STRING_LENGTH - 100) {
        if (strlen(target_text) > 0) strcat(target_text, " ");
        strcat(target_text, current->token);
        current = current->next;
    }
    
    current = reference->tokens->head;
    while (current != NULL && strlen(ref_text) < MAX_STRING_LENGTH - 100) {
        if (strlen(ref_text) > 0) strcat(ref_text, " ");
        strcat(ref_text, current->token);
        current = current->next;
    }
    
    // Simple phrase matching - look for sequences of 3-7 words
    for (int len = 7; len >= 3 && *phrase_count < 5; len--) {
        TokenNode* node = target->tokens->head;
        
        while (node != NULL && *phrase_count < 5) {
            // Build phrase from current position
            char phrase[MAX_TOKEN_LENGTH * 10] = "";
            TokenNode* temp = node;
            int valid_phrase = 1;
            
            for (int i = 0; i < len; i++) {
                if (temp == NULL) {
                    valid_phrase = 0;
                    break;
                }
                if (i > 0) strcat(phrase, " ");
                strcat(phrase, temp->token);
                temp = temp->next;
            }
            
            // Check if this phrase exists in reference
            if (valid_phrase && strstr(ref_text, phrase) != NULL) {
                // Avoid duplicates and substrings
                int is_duplicate = 0;
                for (int i = 0; i < *phrase_count; i++) {
                    if (strstr(phrases[i], phrase) != NULL || strstr(phrase, phrases[i]) != NULL) {
                        is_duplicate = 1;
                        break;
                    }
                }
                
                if (!is_duplicate) {
                    strcpy(phrases[*phrase_count], phrase);
                    (*phrase_count)++;
                }
            }
            
            node = node->next;
        }
    }
}