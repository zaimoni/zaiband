/*
 * grammar.h
 *
 * Copyright 2011, Kenneth Boyd.
 *
 * English grammar support
 *
 * This file is under the Boost License V1.0.
 */

#ifndef GRAMMAR_H
#define GRAMMAR_H 1
 
#include <string.h>

enum gender {
	GENDER_NEUTER = 0,
	GENDER_MALE = 1,
	GENDER_FEMALE = 2
};

#define GENDER_MAX (GENDER_FEMALE+1)

extern const char* const pronoun_subject_third_singular[GENDER_MAX];
extern const char* const pronoun_object_third_singular[GENDER_MAX];
extern const char* const pronoun_possessive_third_singular[GENDER_MAX];
extern const char* const pronoun_reflexive_third_singular[GENDER_MAX];

inline bool is_a_vowel(char ch) {return strchr("aeiouAEIOU",ch);}

#define INDEFINITE_ARTICLE(A) (is_a_vowel(A) ? "an " : "a ")

#endif
