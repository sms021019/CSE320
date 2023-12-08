#include "transaction.h"
#include "debug.h"
#include <stdlib.h>

void trans_init(void){
	info("Initialize transaction manager");
	trans_list.id = 0;
	trans_list.refcnt = 0;
    trans_list.status = TRANS_PENDING;
    trans_list.depends = NULL;
    trans_list.waitcnt = 0;
    pthread_mutex_init(&trans_list.mutex, NULL);
    trans_list.next = &trans_list;
    trans_list.prev = &trans_list;
}

void trans_fini(void){
	info("Finalize transaction manager");
	pthread_mutex_destroy(&trans_list.mutex);
}

TRANSACTION *trans_create(void){
	static unsigned int transaction_id_counter = 0;
	info("Create new transaction %d", transaction_id_counter);

	// Allocate memory for new transaction
	TRANSACTION *new_transaction = malloc(sizeof(TRANSACTION));
	if(new_transaction == NULL){
		error("new transaction malloc failed");
		return NULL;
	}

	// Initialize other fields
	new_transaction->id = __atomic_fetch_add(&transaction_id_counter, 1, __ATOMIC_SEQ_CST);
	new_transaction->status = TRANS_PENDING;
	new_transaction->depends = NULL;
	new_transaction->waitcnt = 0;
	new_transaction->refcnt = 0;
	if(sem_init(&new_transaction->sem, 0,0) != 0){
		error("new transaction semaphore initialization failed");
		free(new_transaction);
		return NULL;
	}
	if(pthread_mutex_init(&new_transaction->mutex, NULL) != 0){
		error("new transaction mutex initialization failed");
		sem_destroy(&new_transaction->sem);
		free(new_transaction);
		return NULL;
	}

	// Increase the reference counter of that transcation
	trans_ref(new_transaction, "newly created transaction");

	//Adding a new transaction into double linked list
	pthread_mutex_lock(&trans_list.mutex);
	new_transaction->next = trans_list.next;
	new_transaction->prev = &trans_list;
	trans_list.next->prev = new_transaction;
	trans_list.next = new_transaction;
	pthread_mutex_unlock(&trans_list.mutex);

	return new_transaction;
}

TRANSACTION *trans_ref(TRANSACTION *tp, char *why){
	info("Increase ref count on transaction %d (%d -> %d) for %s", tp->id, tp->refcnt, tp->refcnt+1, why);
	pthread_mutex_lock(&tp->mutex);
	tp->refcnt++;
	pthread_mutex_unlock(&tp->mutex);
	return tp;
}

void trans_unref(TRANSACTION *tp, char *why){
	info("Decrease ref count on transaction %d (%d -> %d) for %s", tp->id, tp->refcnt, tp->refcnt-1, why);
	pthread_mutex_lock(&tp->mutex);
	tp->refcnt--;

	// Remove the transaction if its reference coutner reached to zero
	if(tp->refcnt == 0){
		info("Free transaction %d", tp->id);
		pthread_mutex_lock(&trans_list.mutex);
		tp->prev->next = tp->next;
		tp->next->prev = tp->prev;
		pthread_mutex_unlock(&trans_list.mutex);

		// Deallocate the dependencies that linked with this transaction
		DEPENDENCY *dp = tp->depends;
		while(dp != NULL){
			DEPENDENCY *temp = dp;
			dp = dp->next;
			free(temp);
		}

		sem_destroy(&tp->sem);
		pthread_mutex_unlock(&tp->mutex);
		pthread_mutex_destroy(&tp->mutex);
		free(tp);
	}else{
		pthread_mutex_unlock(&tp->mutex);
	}
}

void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp){
	// Allocate memory for the new dependency for dtp
	DEPENDENCY *new_dependency = malloc(sizeof(DEPENDENCY));
	if(new_dependency == NULL){
		error("trans_add_dependency new dependency malloc failed");
		free(new_dependency);
		return;
	}

	new_dependency->trans = dtp;

	pthread_mutex_lock(&tp->mutex);
	// Search through tp->depends to check whether the dtp is already in it
	DEPENDENCY *temp = tp->depends;
	while(temp != NULL){
		if(temp->trans == dtp){
			error("trans_add_dependency dtp is already in dependency of tp");
			free(new_dependency);
			return;
		}
		temp = temp->next;
	}

	// Add dtp into tp->depends
	new_dependency->next = tp->depends;
	tp->depends = new_dependency;
	trans_ref(dtp, "adding dependency");
	pthread_mutex_unlock(&tp->mutex);
}

TRANS_STATUS trans_commit(TRANSACTION *tp){
	int abort_flag = 0;
	info("Transaction %d trying to commit", tp->id);
	pthread_mutex_lock(&tp->mutex);
	DEPENDENCY *dp = tp->depends;
	while(dp != NULL){
		TRANSACTION *dependent_trans = dp->trans;

        pthread_mutex_lock(&dependent_trans->mutex);
        dependent_trans->waitcnt++;
        pthread_mutex_unlock(&dependent_trans->mutex);

        sem_wait(&dependent_trans->sem);

        // Check if the dependency transaction is aborted
        if (trans_get_status(dependent_trans) == TRANS_ABORTED) {
            abort_flag = 1;
            break;
        }

        dp = dp->next;
	}
	pthread_mutex_unlock(&tp->mutex);

	if (abort_flag == 0) {
        tp->status = TRANS_COMMITTED;
    } else {
        tp->status = TRANS_ABORTED;
    }

	trans_unref(tp, "attempting to commit transaction");
	return tp->status;
}

TRANS_STATUS trans_abort(TRANSACTION *tp){
	info("Try to abort transaction %d", tp->id);
	pthread_mutex_lock(&tp->mutex);
    if (tp->status == TRANS_COMMITTED) {
        error("trans_abort transaction is commited");
        return -1;
    } else if (tp->status != TRANS_ABORTED) {
        tp->status = TRANS_ABORTED;
        pthread_mutex_unlock(&tp->mutex);

        // Iterate over all transactions and abort dependents
        TRANSACTION *temp = tp->next;
        while (temp->id != tp->id) {
            trans_abort(temp);
            temp = temp->next;
        }

        pthread_mutex_lock(&tp->mutex);
        sem_post(&tp->sem); // Signal any waiting threads
    }
    pthread_mutex_unlock(&tp->mutex);
    info("Transaction %d has aborted", tp->id);
    info("Release %d waiters dependent on transaction %d", tp->waitcnt, tp->id);
    trans_unref(tp, "aborting transaction");
    return TRANS_ABORTED;
}

TRANS_STATUS trans_get_status(TRANSACTION *tp) {
    pthread_mutex_lock(&tp->mutex);
    TRANS_STATUS status = tp->status;
    pthread_mutex_unlock(&tp->mutex);
    return status;
}

void trans_show(TRANSACTION *tp){

}

void trans_show_all(void){

}