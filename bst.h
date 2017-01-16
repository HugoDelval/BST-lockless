template <class T>
class ALIGNEDMA {
public:
    void* operator new(size_t);
    void operator delete(void*);
};

template <class T>
void* ALIGNEDMA<T>::operator new(size_t s){
    return _aligned_malloc(s, lineSz);
}

template <class T>
void ALIGNEDMA<T>::operator delete(void *p){
    _aligned_free(p);
}


class Node: public ALIGNEDMA<Node> {
public:
	INT64 volatile key;
	Node* volatile left;
	Node* volatile right;
	inline Node() {key = 0; right = left = NULL;}
	inline Node(INT64 k) {key = k; right = left = NULL;}
};

class BST: public ALIGNEDMA<BST> {
public:
	Node* volatile root; // root of BST, initially NULL
	volatile int numberOfNodes;
	
	inline BST(){root = NULL; numberOfNodes = 0;}

	inline int add (Node *n) {
		int res;
		ACQUIRE();
		res = 1;
		Node **pp = (Node**)&root;
		Node *p = root;
		while (p) {
			if (n->key < p->key) {
				pp = (Node**)&p->left;
			} else if (n->key > p->key) {
				pp = (Node**)&p->right;
			} else {
				res = 0;
				break;
			}
			p = *pp;
		}
		if(res == 1){
			*pp = n;
		}
		RELEASE();
		if(res == 1)
			InterlockedIncrement64(&numberOfNodes); // to check for memory leak
		return res;
	}

	inline Node* remove(INT64 key) {
		Node *p;
		ACQUIRE();
		Node **pp = (Node**)&root;
		p = root;
		while (p) {
			if (key < p->key) {
				pp = (Node**)&p->left;
			} else if (key > p->key) {
				pp = (Node**)&p->right;
			} else {
				break;
			}
			p = *pp;
		}
		if (p != NULL){		
			if (p->left == NULL && p->right == NULL) {
				*pp = NULL;
			} else if (p->left == NULL) {
				*pp = p->right;
			} else if (p->right == NULL) {
				*pp = p->left;
			} else {
				Node *r = p->right;
				Node **ppr = (Node**)&p->right;
				while (r->left) {
					ppr = (Node**)&r->left;
					r = r->left;
				}
				p->key = r->key;
				p = r;
				*ppr = r->right;
			}
		}
		RELEASE();
		if (p != NULL)
			InterlockedExchangeAdd64(&numberOfNodes, -1); // to check for memory leak
		return p;
	}
	
	inline void prefill(UINT64 keyRange){
		UINT64 randomNumber = getWallClockMS();
		for(int i=0 ; i<(keyRange*2.1)/3 ; ++i){ // tree filled at ~50% 
			randomNumber = rand(randomNumber);
			INT64 key = (randomNumber >> 1) & (keyRange-1); // <=> % keyRange;
			add(new Node(key));
		}
		// percentage filled :
		// cout << numberOfNodes/(double)keyRange << endl;
	}


	/***********************************************************
	** following functions are used to check tree consistency **
	***********************************************************/ 
	
	inline UINT64 countNbOfNodes(Node* start) {
		if(start == NULL){
			cout << "Tree empty !" << endl;
			return 0;
		}
		UINT64 right=0, left=0;
		if(start->right != NULL){
			right = countNbOfNodes(start->right);
		}
		if(start->left != NULL){
			left = countNbOfNodes(start->left);
		}
		return 1 + right + left;
	}

	inline bool treeOrderConsistent(Node* start){
		if(start == NULL){
			cout << "Tree empty !" << endl;
			return true;
		}
		bool right=true, left=true;
		if(start->right != NULL){
			if(start->right->key <= start->key){
				cout << start->right->key << " is on the right of " << start->key << endl;
				return false;
			}
			right &= treeOrderConsistent(start->right);
		}
		if(start->left != NULL){
			if(start->left->key >= start->key){
				cout << start->left->key << " is on the right of " << start->key << endl;
				return false;
			}
			left &= treeOrderConsistent(start->left);
		}
		return right && left;
	}

	inline bool treeIsConsistent(){
		UINT64 countedNbOfNodes = countNbOfNodes(root);
		if(countedNbOfNodes != numberOfNodes){
			cout << "(countNbOfNodes(root) = " << countedNbOfNodes << ") != (numberOfNodes = " << numberOfNodes << ")" << endl;
			return false;
		}
		return treeOrderConsistent(root);
	}

};
