#define __STDC_WANT_LIB_EXT1__ 1

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

int vector_setup(Vector* vector, size_t capacity, size_t element_size) {
	assert(vector != NULL);
	assert(!vector_is_initialized(vector));

	if (vector == NULL) return VECTOR_ERROR;
	if (vector_is_initialized(vector)) return VECTOR_ERROR;

	vector->size = 0;
	vector->capacity = MAX(VECTOR_MINIMUM_CAPACITY, capacity);
	vector->element_size = element_size;
	vector->data = malloc(capacity * element_size);

	return vector->data ? VECTOR_SUCCESS : VECTOR_ERROR;
}

int vector_destroy(Vector* vector) {
	assert(vector != NULL);
	assert(vector_is_initialized(vector));

	if (vector == NULL) return VECTOR_ERROR;
	if (!vector_is_initialized(vector)) return VECTOR_ERROR;

	free(vector->data);

	return VECTOR_SUCCESS;
}

// Insertion
int vector_push_back(Vector* vector, void* element) {
	assert(vector != NULL);
	assert(element != NULL);

	if (_vector_should_grow(vector)) {
		if (_vector_adjust_capacity(vector) == VECTOR_ERROR) {
			return VECTOR_ERROR;
		}
	}

	_vector_assign(vector, vector->size, element);

	++vector->size;

	return VECTOR_SUCCESS;
}

int vector_push_front(Vector* vector, void* element) {
	return vector_insert(vector, 0, element);
}

int vector_insert(Vector* vector, size_t index, void* element) {
	void* offset;

	assert(vector != NULL);
	assert(element != NULL);
	assert(index <= vector->size);

	if (vector == NULL) return VECTOR_ERROR;
	if (element == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;
	if (index > vector->size) return VECTOR_ERROR;

	if (_vector_should_grow(vector)) {
		if (_vector_adjust_capacity(vector) == VECTOR_ERROR) {
			return VECTOR_ERROR;
		}
	}

	// Move other elements to the right
	offset = _vector_offset(vector, index);
	if (_vector_move_right(vector, index, offset) == VECTOR_ERROR) {
		return VECTOR_ERROR;
	}

	// Insert the element
	memcpy(offset, element, vector->element_size);
	++vector->size;

	return VECTOR_SUCCESS;
}

int vector_assign(Vector* vector, size_t index, void* element) {
	assert(vector != NULL);
	assert(element != NULL);
	assert(index < vector->size);

	if (vector == NULL) return VECTOR_ERROR;
	if (element == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;
	if (index >= vector->size) return VECTOR_ERROR;

	_vector_assign(vector, index, element);

	return VECTOR_SUCCESS;
}

// Deletion
int vector_pop_back(Vector* vector) {
	assert(vector != NULL);
	assert(vector->size > 0);

	if (vector == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;

	--vector->size;

#ifndef VECTOR_NO_SHRINK
	if (_vector_should_shrink(vector)) {
		_vector_adjust_capacity(vector);
	}
#endif

	return VECTOR_SUCCESS;
}

int vector_pop_front(Vector* vector) {
	return vector_remove(vector, 0);
}

int vector_remove(Vector* vector, size_t index) {
	assert(vector != NULL);
	assert(index < vector->size);

	if (vector == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;
	if (index >= vector->size) return VECTOR_ERROR;

	// Just overwrite
	_vector_move_left(vector, index);

#ifndef VECTOR_NO_SHRINK
	if (--vector->size == vector->capacity / 4) {
		_vector_adjust_capacity(vector);
	}
#endif

	return VECTOR_SUCCESS;
}

int vector_clear(Vector* vector) {
	return vector_resize(vector, 0);
}

// Lookup
void* vector_get(Vector* vector, size_t index) {
	assert(vector != NULL);
	assert(index < vector->size);

	if (vector == NULL) return NULL;
	if (vector->element_size == 0) return NULL;
	if (index >= vector->size) return NULL;

	return _vector_offset(vector, index);
}

void* vector_front(Vector* vector) {
	return vector_get(vector, 0);
}

void* vector_back(Vector* vector) {
	return vector_get(vector, vector->size - 1);
}

// Information

bool vector_is_initialized(const Vector* vector) {
	return vector->data != NULL;
}

size_t vector_byte_size(const Vector* vector) {
	return vector->size * vector->element_size;
}

size_t vector_free_space(const Vector* vector) {
	return vector->capacity - vector->size;
}

bool vector_is_empty(const Vector* vector) {
	return vector->size == 0;
}

// Memory management
int vector_resize(Vector* vector, size_t new_size) {
	if (new_size > vector->capacity ||
			new_size <= vector->size * VECTOR_SHRINK_THRESHOLD) {
		if (_vector_reallocate(vector, new_size * VECTOR_GROWTH_FACTOR) == -1) {
			return VECTOR_ERROR;
		}
	}

	vector->size = new_size;

	return VECTOR_SUCCESS;
}

int vector_reserve(Vector* vector, size_t minimum_capacity) {
	if (minimum_capacity > vector->capacity) {
		if (_vector_reallocate(vector, minimum_capacity) == VECTOR_ERROR) {
			return VECTOR_ERROR;
		}
	}

	return VECTOR_SUCCESS;
}

int vector_shrink_to_fit(Vector* vector) {
	return _vector_reallocate(vector, vector->size);
}

// Iterators
Iterator vector_begin(Vector* vector) {
	return vector_iterator(vector, 0);
}

Iterator vector_end(Vector* vector) {
	return vector_iterator(vector, vector->size);
}

Iterator vector_iterator(Vector* vector, size_t index) {
	Iterator iterator = {NULL, 0};

	assert(vector != NULL);
	assert(index <= vector->size);

	if (vector == NULL) return iterator;
	if (index > vector->size) return iterator;
	if (vector->element_size == 0) return iterator;

	iterator.pointer = _vector_offset(vector, index);
	iterator.element_size = vector->element_size;

	return iterator;
}

void* iterator_get(Iterator* iterator) {
	return iterator->pointer;
}

void iterator_increment(Iterator* iterator) {
	assert(iterator != NULL);
	iterator->pointer += iterator->element_size;
}

void iterator_decrement(Iterator* iterator) {
	assert(iterator != NULL);
	iterator->pointer -= iterator->element_size;
}

void* iterator_next(Iterator* iterator) {
	void* current = iterator->pointer;
	iterator_increment(iterator);

	return current;
}

void* iterator_previous(Iterator* iterator) {
	void* current = iterator->pointer;
	iterator_decrement(iterator);

	return current;
}

bool iterator_equals(Iterator* first, Iterator* second) {
	assert(first->element_size == second->element_size);
	return first->pointer == second->pointer;
}

bool iterator_is_before(Iterator* first, Iterator* second) {
	assert(first->element_size == second->element_size);
	return first->pointer < second->pointer;
}

bool iterator_is_after(Iterator* first, Iterator* second) {
	assert(first->element_size == second->element_size);
	return first->pointer > second->pointer;
}

/***** PRIVATE *****/

bool _vector_should_grow(Vector* vector) {
	assert(vector->size <= vector->capacity);
	return vector->size == vector->capacity;
}

bool _vector_should_shrink(Vector* vector) {
	assert(vector->size <= vector->capacity);
	return vector->size == vector->capacity * VECTOR_SHRINK_THRESHOLD;
}

size_t _vector_free_bytes(const Vector* vector) {
	return vector_free_space(vector) * vector->element_size;
}

void* _vector_offset(Vector* vector, size_t index) {
	return vector->data + (index * vector->element_size);
}

void _vector_assign(Vector* vector, size_t index, void* element) {
	// Insert the element
	void* offset = _vector_offset(vector, index);
	memcpy(offset, element, vector->element_size);
}

int _vector_move_right(Vector* vector, size_t index, void* offset) {
	// How many to move to the right
	size_t elements_in_bytes = (vector->size - index) * vector->element_size;

#ifdef __STDC_LIB_EXT1__
	size_t right_capacity_in_bytes =
			(vector->capacity - (index + 1)) * vector->element_size;
	// clang-format off
	return memmove_s(
		offset + vector->element_size,
		right_capacity_in_bytes,
		offset,
		elements_in_bytes
	);
// clang-format on
#else
	memmove(offset + vector->element_size, offset, elements_in_bytes);
	return VECTOR_SUCCESS;
#endif
}

void _vector_move_left(Vector* vector, size_t index) {
	size_t right_elements_in_bytes;
	void* offset;

	// The offset into the memory
	offset = _vector_offset(vector, index);

	// How many to move to the left
	right_elements_in_bytes = (vector->size - index - 1) * vector->element_size;

	memmove(offset, offset + vector->element_size, right_elements_in_bytes);
}

int _vector_adjust_capacity(Vector* vector) {
	return _vector_reallocate(vector, vector->size * VECTOR_GROWTH_FACTOR);
}

int _vector_reallocate(Vector* vector, size_t new_capacity) {
	size_t new_capacity_in_bytes;
	void* old;
	assert(vector != NULL);

	if (new_capacity < VECTOR_MINIMUM_CAPACITY) {
		if (vector->capacity > VECTOR_MINIMUM_CAPACITY) {
			new_capacity = VECTOR_MINIMUM_CAPACITY;
		} else {
			// NO-OP
			return VECTOR_SUCCESS;
		}
	}

	new_capacity_in_bytes = new_capacity * vector->element_size;
	old = vector->data;

	if ((vector->data = malloc(new_capacity_in_bytes)) == NULL) {
		return VECTOR_ERROR;
	}

#ifdef __STDC_LIB_EXT1__
	if (memcpy_s(vector->data,
							 new_capacity_in_bytes,
							 old,
							 vector_byte_size(vector, vector->element_size)) != 0) {
		return VECTOR_ERROR;
	}
#else
	memcpy(vector->data, old, vector_byte_size(vector));
#endif

	vector->capacity = new_capacity;

	free(old);

	return VECTOR_SUCCESS;
}
