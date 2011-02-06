/*
 * grammar.c
 *
 * Copyright 2011, Kenneth Boyd.
 *
 * English grammar support
 *
 * This file is under the Boost License V1.0.
 */

#include "grammar.h"
 
const char* const pronoun_subject_third_singular[GENDER_MAX] = { "it", "he", "she" };
const char* const pronoun_object_third_singular[GENDER_MAX] = { "it", "him", "her" };
const char* const pronoun_possessive_third_singular[GENDER_MAX] = { "its", "his", "her" };
const char* const pronoun_reflexive_third_singular[GENDER_MAX] = { "itself", "himself", "herself" };

