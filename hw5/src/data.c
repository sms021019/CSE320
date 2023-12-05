#include "data.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


BLOB *blob_create(char *content, size_t size){
	BLOB *new_blob = malloc(sizeof(BLOB));
	if(new_blob == NULL){
		debug("new_blob malloc failed");
		return NULL;
	}
	info("Create blob with content %p, size %zu -> %p", (void *)content, size, (void *)new_blob);
	pthread_mutex_init(&new_blob->mutex, NULL);

	new_blob->size = size;
	// Allocate memory and copy the content
    new_blob->content = malloc(size+1);
    new_blob->prefix = malloc(size+1);
    if (new_blob->content == NULL || new_blob->prefix == NULL) {
    	debug("new_blob content malloc failed");
        pthread_mutex_destroy(&new_blob->mutex);
        free(new_blob);
        return NULL;
    }
    // debug("content: %s", content);
    // debug("size: %ld", size);
    memcpy(new_blob->content, content, size);
    new_blob->content[size] = '\0';
    memcpy(new_blob->prefix, content, size);
    new_blob->prefix[size] = '\0';

    // debug("new_blob->content: %s", new_blob->content);
	blob_ref(new_blob, "newly created blob");

	return new_blob;
}

BLOB *blob_ref(BLOB *bp, char *why){
	info("Increase reference count on a blob %p [%s] (%d -> %d) for %s", (void *)bp, bp->content, bp->refcnt, bp->refcnt + 1, why);
	pthread_mutex_lock(&bp->mutex);
	bp->refcnt++;
	pthread_mutex_unlock(&bp->mutex);
	return bp;
}

void blob_unref(BLOB *bp, char *why){
	info("Decrease reference count on a blob %p [%s] (%d -> %d) for %s", (void *)bp, bp->content, bp->refcnt, bp->refcnt - 1, why);
	pthread_mutex_lock(&bp->mutex);
	bp->refcnt--;
	pthread_mutex_unlock(&bp->mutex);
	if(bp->refcnt == 0){
		info("Free blob %p [%s]", (void *)bp, bp->content);
		if(bp->prefix != NULL){
			free(bp->prefix);
		}
		free(bp->content);
		pthread_mutex_destroy(&bp->mutex);
		free(bp);
	}
}

int blob_compare(BLOB *bp1, BLOB *bp2){
	if(strcmp(bp1->content, bp2->content) == 0){
		info("Result: Same");
		return 0;
	}else{
		info("Result: Different");
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
	if(new_key == NULL){
		error("new_key malloc failed");
		return NULL;
	}

	new_key->blob = (void *)bp;
	new_key->hash = blob_hash(bp);

	info("Create key from blob %p -> %p [%s]", (void *)bp, new_key, new_key->blob->content);

	return new_key;
}

void key_dispose(KEY *kp){
	info("Dispose of key %p [%s]", (void *)kp, kp->blob->content);
	if(kp->blob != NULL){
		blob_unref(kp->blob, "blob in key");
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
	info("Create version of blob %p [%s] for transaction %d -> %p", (void *)bp, bp->content, tp->id, (void *)new_version);
	trans_ref(tp, "creator of version");

	return new_version;
}

void version_dispose(VERSION *vp){
	info("Dispose of version %p", (void*)vp);
	if(vp->blob != NULL){
		trans_unref(vp->creator, "creator of version");
		blob_unref(vp->blob, "blob in version");
		vp->blob = NULL;
	}
	free(vp);
}

