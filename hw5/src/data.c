#include "data.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

BLOB *blob_create(char *content, size_t size){
	BLOB *new_blob = malloc(sizeof(BLOB));
	pthread_mutex_init(&new_blob->mutex, NULL);

	new_blob->size = size;
	// Allocate memory and copy the content
    new_blob->content = malloc(size);
    if (new_blob->content == NULL) {
        // Handle memory allocation failure
        pthread_mutex_destroy(&new_blob->mutex);
        free(new_blob);
        return NULL;
    }
    memcpy(new_blob->content, content, size);

	new_blob->refcnt = 1;

	return new_blob;
}

BLOB *blob_ref(BLOB *bp, char *why){
	debug("%s | Increase the reference count on a blob from (%d) to (%d)\n", why, bp->refcnt, bp->refcnt + 1);
	pthread_mutex_lock(&bp->mutex);
	bp->refcnt++;
	pthread_mutex_unlock(&bp->mutex);
	return bp;
}

void blob_unref(BLOB *bp, char *why){
	debug("%s | Decrease the reference count on a blob from (%d) to (%d)\n", why, bp->refcnt, bp->refcnt - 1);
	pthread_mutex_lock(&bp->mutex);
	bp->refcnt--;
	pthread_mutex_unlock(&bp->mutex);
	if(bp->refcnt == 0){
		debug("Reference count of a blob is reached 0.\nFreeing this blob...\n");
		if(bp->prefix != NULL){
			free(bp->prefix);
		}
		free(bp->content);
		pthread_mutex_destroy(&bp->mutex);
		free(bp);
	}
}

int blob_compare(BLOB *bp1, BLOB *bp2){
	debug("Compare two blobs for equality of their content\n");
	if(strcmp(bp1->content, bp2->content) == 0){
		debug("Result: Same\n");
		return 0;
	}else{
		debug("Result: Different");
		return -1;
	}
}

int blob_hash(BLOB *bp){
	unsigned long hash = 5381;
    int c;

    for (size_t i = 0; i < bp->size; i++) {
        c = bp->content[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

KEY *key_create(BLOB *bp){
	KEY *new_key = malloc(sizeof(KEY));
	new_key->blob = bp;
	new_key->hash = blob_hash(bp);

	return new_key;
}

void key_dispose(KEY *kp){
	if(kp->blob != NULL){
		blob_unref(kp->blob, "KEY DISPOSE");
		kp->blob = NULL;
	}
	free(kp);
}

int key_compare(KEY *kp1, KEY *kp2){
	if(kp1 == kp2){
		return 0;
	}

	if(blob_compare(kp1->blob, kp2->blob) == 0){
		if(kp1->hash == kp2->hash){
			return 0;
		}
	}

	return -1;
}

VERSION *version_create(TRANSACTION *tp, BLOB *bp){
	VERSION *new_version = malloc(sizeof(VERSION));
	new_version->creator = tp;
	new_version->blob = bp;
	blob_ref(bp, "VERSION CREATE");

	return new_version;
}

void version_dispose(VERSION *vp){
	if(vp->blob != NULL){
		blob_unref(vp->blob, "VERSION DISPOSE");
		vp->blob = NULL;
	}
	free(vp);
}

