/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <string.h>
#include <stdalign.h>

#include "../api.h"
#include "vertex_buffer.h"
#include "core.h"

void r_vertex_buffer_create(VertexBuffer *vbuf, size_t capacity, void *data) {
	memset(vbuf, 0, sizeof(VertexBuffer));
	vbuf->impl = calloc(1, sizeof(VertexBufferImpl));
	vbuf->size = capacity;

	glGenBuffers(1, &vbuf->impl->gl_handle);

	GLuint vbo_saved = gl33_vbo_current();
	gl33_bind_vbo(vbuf->impl->gl_handle);
	gl33_sync_vbo();
	glBufferData(GL_ARRAY_BUFFER, capacity, data, GL_STATIC_DRAW);
	gl33_bind_vbo(vbo_saved);
}

void r_vertex_buffer_destroy(VertexBuffer *vbuf) {
	if(vbuf->impl) {
		gl33_vertex_buffer_deleted(vbuf);
		glDeleteBuffers(1, &vbuf->impl->gl_handle);
		free(vbuf->impl);
		vbuf->impl = NULL;
	}
}

void r_vertex_buffer_invalidate(VertexBuffer *vbuf) {
	assert(vbuf->impl != NULL);

	GLuint vbo_saved = gl33_vbo_current();
	gl33_bind_vbo(vbuf->impl->gl_handle);
	gl33_sync_vbo();
	glBufferData(GL_ARRAY_BUFFER, vbuf->size, NULL, GL_DYNAMIC_DRAW);
	gl33_bind_vbo(vbo_saved);

	vbuf->offset = 0;
}

void r_vertex_buffer_write(VertexBuffer *vbuf, size_t offset, size_t data_size, void *data) {
	assert(vbuf->impl != NULL);
	assert(data_size > 0);
	assert(offset + data_size <= vbuf->size);

	GLuint vbo_saved = gl33_vbo_current();
	gl33_bind_vbo(vbuf->impl->gl_handle);
	gl33_sync_vbo();
	glBufferSubData(GL_ARRAY_BUFFER, offset, data_size, data);
	gl33_bind_vbo(vbo_saved);
}

void r_vertex_buffer_append(VertexBuffer *vbuf, size_t data_size, void *data) {
	// log_debug("%u -> %u / %u", (uint)vbuf->offset, (uint)(vbuf->offset + data_size), (uint)vbuf->size);
	r_vertex_buffer_write(vbuf, vbuf->offset, data_size, data);
	vbuf->offset += data_size;
}
