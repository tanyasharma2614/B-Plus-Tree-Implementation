#include "include/b_plus_tree.h"
#include <iostream>


/*
 * Helper function to decide whether current b+tree is empty
 */
bool BPlusTree::IsEmpty() const { 
    //check if root node is empty 
    if(root==NULL)
        return true; 
    else
        return false;
}

void BPlusTree:: Display( InternalNode* selected) {
    if(IsEmpty())
        return;

        for(int i=0;i<selected->key_num;i++){
            cout<<selected->keys[i]<<" ";
        }
        cout<<"\n";

        if(selected->is_leaf!=true){
            for(int i=0;i<selected->key_num+1;i++){
              Display((InternalNode*)selected->children[i]);
            }
        }
}

Node* BPlusTree::getRoot(){
    return root;
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
bool BPlusTree::GetValue(const KeyType &key,  RecordPointer &result) { 
    
    //if root is NULL, tree is empty and we return false (search not found)
    if(root==NULL)
        return false;
    //if root is not NULL and is not a LeafNode, we use a technique similar to binary search for the current node to traverse till the relevant leaf node
    InternalNode *selected = (InternalNode*)root;

    while(selected->is_leaf == false){

        for(int x=0;x<selected->key_num;x++){

            if(key<selected->keys[x]){
                selected=(InternalNode*)selected->children[x];
                break;
            }

            if(x == selected->key_num-1){
                selected=(InternalNode*)selected->children[x+1];
                break;
            }
        }
    }

    //if root is a LeafNode, we check if key is found and return the record
    LeafNode *current = (LeafNode*)selected;

    for(long int i=0;i<current->key_num;i++){
        if(current->keys[i]==key){
            result.page_id=current->pointers[i].page_id;
            result.record_id=current->pointers[i].record_id;
            return true;
        }
    }
    return false; 
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * If current tree is empty, start new tree, otherwise insert into leaf Node.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */


bool BPlusTree::Insert(const KeyType &key, const RecordPointer &value) {

    RecordPointer result;
    //only unique keys can be inserted
    if(GetValue(key,result))
        return false;

    //if tree is empty, create an empty leaf node L, which is also the root
    if(IsEmpty()){
        root=insertNewRoot(key,value);
        return true;
    }

    //finding leaf node L that should contain key value K
    else{

        //find the position of insertion of the new node
        InternalNode *current = (InternalNode*) root;
        InternalNode *previous;

        //traversing all internal nodes till we reach a relevant leaf node 
        while(current->is_leaf==false){

            //previous becomes current
            previous=current;

            //checking all children of the current node
			for(int x = 0;x<current->key_num;x++){

                //if key is less than than the current key,we move to the left child (go down that to that parent's children)
                if(key<current->keys[x]){
                    //current node becomes the child node and we traverse left
                    current=(InternalNode*)current->children[x];
                    break;
                }

                 //if we have reached the end of the list of children, position not found, we go to next child
                if(x==current->key_num-1){
                    current=(InternalNode*)current->children[x+1];
                    break;
                }
               

            }
        }

        //we have reached a leaf node
        LeafNode *currentLeaf=(LeafNode*) current;

        //if L has less than n-1 keys
        //if number of children is still less than max permissible value even if we add one more node (No overflow)
        if(currentLeaf->key_num<MAX_FANOUT-1){
            long int x=0;

            //as long as key value is greater than that of current key
            while(key>currentLeaf->keys[x] &&  x<currentLeaf->key_num)
            {
                x++;
            }

            //now, x is at the correct insert position of the new node
            for(long int y=currentLeaf->key_num;y>x;y--){
                //shifting nodes to the right
                currentLeaf->pointers[y]=currentLeaf->pointers[y-1];  
                currentLeaf->keys[y]=currentLeaf->keys[y-1];
            }

            //now node at position x is ready to be inserted and it wil be a leaf node
            currentLeaf->key_num++;
            currentLeaf->pointers[x]=value;
            currentLeaf->keys[x]=key;
            // currentLeaf=insertInLeaf(key,value,currentLeaf);
    }

        //L has n-1 key values already, split it
        //Leaf node has max permissible keys so we split it
        else{

            //create node L'
            //Previous leaf node is full and we need to create a new one
            LeafNode *newLeafNode = new LeafNode();
            //array to store key and values of current leaf node
            KeyType tempNode[MAX_FANOUT];
            RecordPointer tempValues[MAX_FANOUT];

            //copy L.P1....L.Kn-1 to a block of memory T that can hold n(pointer,key-value) pairs
            //copying key,value pairs to a temporary block of memory 
            for(int x=0;x<MAX_FANOUT-1;x++){
                tempValues[x]=currentLeaf->pointers[x];
                tempNode[x]=currentLeaf->keys[x];
            }

            //finding correct position
            long int x=0,y;
            while(key>tempNode[x] && x<MAX_FANOUT-1)
                x++;

            /*check if out of bounfd*/
            //shifting keys and values to the right 
			for(int y = MAX_FANOUT-1;y>x;y--){ 
                tempValues[y]=tempValues[y-1];
                tempNode[y]=tempNode[y-1];
            }

            //inserting key and value at their correct position
            newLeafNode->is_leaf=true;
            tempValues[x]=value;
            tempNode[x]=key;

            //splitting the current leaf to store the first n/2 values
            currentLeaf->key_num=(MAX_FANOUT)/2;
            //new leaf will store the remaining values
            newLeafNode->key_num=MAX_FANOUT-(MAX_FANOUT)/2;

            //saving keys and values for the first half of the split node in the current node
			for(x = 0;x<currentLeaf->key_num;x++){
                currentLeaf->pointers[x]=tempValues[x];
                currentLeaf->keys[x]=tempNode[x];
            }

            //saving keys and values for the second half of the split node in the new leaf
			for(x=0, y= current->key_num; x<newLeafNode->key_num; x++,y++){
                newLeafNode->pointers[x]=tempValues[y];
                newLeafNode->keys[x]=tempNode[y];
            }

            //assigning pointers to the split node
            if(currentLeaf->next_leaf==NULL){
            currentLeaf->next_leaf=newLeafNode;
            newLeafNode->prev_leaf=currentLeaf; 
            }
            else{
                LeafNode *tempLeaf=currentLeaf->next_leaf;
                currentLeaf->next_leaf=newLeafNode;
                newLeafNode->prev_leaf=currentLeaf;
                newLeafNode->next_leaf=tempLeaf;
            }

            //if the current leaf is the root, then we create a new root and assign it to the root variable
            if(currentLeaf==root){
                root=insertRoot(currentLeaf,newLeafNode);
            }

            //if parent is not a root node, we traverse the tree upto the root, making changes as we go
            else{
                insertInternal(newLeafNode->keys[0],previous, (InternalNode*) newLeafNode);
            }
        }
    }
    return true; 
}

void BPlusTree::insertInternal(const KeyType &x, InternalNode *parent, InternalNode *child){

    //current node is the previous node for the newly split leaf nodes and child is the new leaf node
    InternalNode *current=parent;

    //if current node is not full, we can insert value there itself
    if(current->key_num<MAX_FANOUT-1){
        long int i=0;

        //finding correct position
		while(x > current->keys[i] && i<current->key_num){
            i++;
        }

        //shifting keys to the right to make place 
		for(int j = current->key_num; j>i; j--){
            current->keys[j]=current->keys[j-1];
        }

        //shifting child pointers appropriately
		for(int j = current->key_num+1;j>i+1;j--){
            current->children[j]=(InternalNode*)(current->children[j-1]);
        }

        current->key_num++;
        current->keys[i]=x;
        current->children[i+1]=(InternalNode*)child;
        // current=insertInternalNode(x, current,child);
    
    }

    //if current node is full, we split again
    else{

        InternalNode *newInternal = new InternalNode();
        KeyType tempKey[MAX_FANOUT];
        Node *tempNode[MAX_FANOUT+1];

		for(int i=0; i<MAX_FANOUT-1;i++){
            tempKey[i]=current->keys[i];
        }

        //storing child pointer values
		for(int i=0; i<MAX_FANOUT;i++){
            tempNode[i]=(InternalNode*)current->children[i];
        }

        int i=0,j;
        //finding correct position for split
        while(x>tempKey[i] && i<MAX_FANOUT-1){
            i++;
        }

        //moving keys to make place for insertion 
		for(int j = MAX_FANOUT-1; j>i; j--){
            tempKey[j]=tempKey[j-1];
        }

        //storing key in correct position
        tempKey[i]=x;

        //moving pointers to the right
		for(int j = MAX_FANOUT;j>i+1; j--){
            tempNode[j]=tempNode[j-1];
        }

        //storing pointer at correct position
        newInternal->is_leaf=false;
        tempNode[i+1]=(InternalNode*)child;

        //splitting the node into two nodes
        current->key_num=MAX_FANOUT/2;
        newInternal->key_num=(MAX_FANOUT) - (MAX_FANOUT)/2-1;


		for(i = 0, j = current->key_num+1; i<newInternal->key_num && j<MAX_FANOUT;i++, j++){
            newInternal->keys[i]=tempKey[j];
        }

 
		for(i=0, j=current->key_num+1;i<newInternal->key_num+1;i++, j++){
            newInternal->children[i]=(InternalNode*)tempNode[j];
        }

        if(current==(InternalNode*)root){

            InternalNode *currentR=(InternalNode*)current;

            root=insertRootInternal(currentR,newInternal,tempKey);
        }

        else{
            insertInternal(current->keys[current->key_num],findParent(root,current),newInternal);
        }
    }
}

Node* BPlusTree::insertNewRoot(const KeyType &key, const RecordPointer &value){
        
        //create new node if root is empty and insert key/value pair into the node
        LeafNode *newRootNode = new LeafNode();
        newRootNode->is_leaf=true;
        newRootNode->key_num=1;
        newRootNode->pointers[newRootNode->key_num-1]=value;
        newRootNode->keys[newRootNode->key_num-1]=key;
        root=newRootNode; 
        return root;
}

Node* BPlusTree:: insertRoot(LeafNode *currentLeaf, LeafNode *newLeafNode){
        //if current leaf is the root
        InternalNode *newRoot = new InternalNode();
        newRoot->key_num=1;
        newRoot->is_leaf=false;
        newRoot->keys[newRoot->key_num-1]=newLeafNode->keys[newRoot->key_num-1];
        newRoot->children[newRoot->key_num-1]=currentLeaf;
        newRoot->children[newRoot->key_num]=newLeafNode;
        root=(Node*)newRoot;
        return root;
        
}

Node* BPlusTree:: insertRootInternal(InternalNode *currentR, InternalNode *newInternal, int tempKey[]){
            InternalNode *newInternalNode = new InternalNode();
            newInternalNode->is_leaf=false;
            newInternalNode->key_num=1;
            newInternalNode->keys[newInternalNode->key_num-1]=tempKey[MAX_FANOUT/2];
            newInternalNode->children[newInternalNode->key_num-1]=currentR;
            newInternalNode->children[newInternalNode->key_num]=newInternal;
            root=(Node*) newInternalNode;
            return root;
}

LeafNode* BPlusTree:: insertInLeaf(const KeyType &key, const RecordPointer &value, LeafNode *currentLeaf){

            long int x=0;

            //as long as key value is greater than that of current key
            while(key>currentLeaf->keys[x] &&  x<currentLeaf->key_num)
            {
                x++;
            }

            //now, x is at the correct insert position of the new node
            for(long int y=currentLeaf->key_num;y>x;y--){
                //shifting nodes to the right
                currentLeaf->pointers[y]=currentLeaf->pointers[y-1];  
                currentLeaf->keys[y]=currentLeaf->keys[y-1];
            }

            //now node at position x is ready to be inserted and it wil be a leaf node
            currentLeaf->key_num++;
            currentLeaf->pointers[x]=value;
            currentLeaf->keys[x]=key;
            return currentLeaf;
}

InternalNode* BPlusTree:: insertInternalNode(const KeyType &x, InternalNode *current,InternalNode *child){
        long int i=0;
        long int j=0;

        //finding correct position
        while(x>current->keys[i] && i<current->key_num)
            i++;

        //shifting keys to the right to make place 
        j=current->key_num;
        while(j>i){
            current->keys[j]=current->keys[j-1];
            j--;
        }

        //shifting child pointers appropriately
        j=current->key_num+1;
        while(j>i+1){
            current->children[j]=(InternalNode*)(current->children[j-1]);
            j--;
        }

        current->key_num++;
        current->keys[i]=x;
        current->children[i+1]=(InternalNode*)child;
        return current;
}

InternalNode* BPlusTree::findParent(Node *root, InternalNode *child){
    InternalNode *parent;
    InternalNode *temp=(InternalNode*)root;
    
    //if node is a leaf node, or if the child is a leaf node, return NULL
    if(temp->is_leaf || temp->children[0]->is_leaf){
        return NULL;
    }

    //traversing all children to find match
	for(int i=0;i<temp->key_num+1;i++){
         if(temp->children[i]==child){
            parent=temp;
            return parent;
        }
        else{
            parent=findParent(temp->children[i],child);
            if(parent!=NULL)
                return parent;
            }
    }
    return parent;
}
/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immdiately.
 * If not, User needs to first find the right leaf node as deletion target, then
 * delete entry from leaf node. Remember to deal with redistribute or merge if
 * necessary.
 */
void BPlusTree::Remove(const KeyType &key) {
    RecordPointer record;
    //if tree is empty or value not found, we return back and not execute the rest of the function
    if(IsEmpty() || !GetValue(key, record))
        return;
    
    //if key is present, we execute remove function
    InternalNode *current = (InternalNode*)root;
    InternalNode *previous;
    long int child[2];

    //find lead node L that contains key value pair
    //while the node is not a leaf, we drill down in the correct sequence to find the leaf node that needs to be removed
    while(current->is_leaf == false){
        for(long int i=0;i<current->key_num;i++){

            previous=current;
            child[0]= i - 1;
            child[1]= i + 1;

            //comparing value of element with key values of node to perform a type of binary search
            if(key<current->keys[i]){
                current=(InternalNode*)current->children[i];
                break;
            }

            if(i == current->key_num-1){
                child[0]=i;
                child[1]=i+2;
                current=(InternalNode*)current->children[i+1];
                break;
            }
        }
    }
    //by executing the above search, we have reached the correct leaf node
    LeafNode *currentLeaf = (LeafNode*) current;
    long int position,i; 

    //finding position
    position=findRemoveNode(key,currentLeaf);

    i=position;
    while(i+1<currentLeaf->key_num){
        currentLeaf->keys[i]=currentLeaf->keys[i+1];
        currentLeaf->pointers[i]=currentLeaf->pointers[i+1];
        i++;
    }
    
    currentLeaf->key_num--;
    
    //if the leaf node is the root, tree is empty after removal
    if(currentLeaf==(LeafNode*)root){
            if(currentLeaf->key_num==0){
            root = NULL;
        }
        return;
    }

    //we know that for any node, half of the keys should be full
    if(currentLeaf->key_num>=(MAX_FANOUT)/2){
        return;
    }

    //if left child is not empty
    if(child[0]>=0){
        LeafNode* leftNode = (LeafNode*) previous->children[child[0]];
        if(leftNode->key_num>=(MAX_FANOUT)/2+1){

            long int i=currentLeaf->key_num;
            while(i>0){
                currentLeaf->keys[i]=currentLeaf->keys[i-1];
                currentLeaf->pointers[i]=currentLeaf->pointers[i-1];
                i--;
            }

            currentLeaf->key_num++;
            currentLeaf->keys[0] =leftNode->keys[leftNode->key_num-1];
            leftNode->key_num--;
            leftNode->next_leaf=currentLeaf;
            previous->keys[child[0]]=currentLeaf->keys[0];
            return;
        }
        
    }
    if(child[1]<=previous->key_num){

        LeafNode *rightNode = (LeafNode*) previous->children[child[1]];
        if(rightNode->key_num>=(MAX_FANOUT)/2+1){
            current->key_num++;
            currentLeaf->keys[currentLeaf->key_num-1] = rightNode->keys[0];
            currentLeaf->pointers[currentLeaf->key_num-1] = rightNode->pointers[0];
            rightNode->key_num--;
            rightNode->prev_leaf=currentLeaf;

            long int i=0;
            while(i<rightNode->key_num){
                rightNode->keys[i] = rightNode->keys[i+1];
                i++;
            }

            previous->keys[child[1]-1]=rightNode->keys[0];
            return;
        }
    }

    if(child[0]>=0){
        handleLeftChild(previous,currentLeaf,child[0]);
    }

    else if(child[1]<=previous->key_num){
        handleRightChild(previous,currentLeaf,child[1]);
    }


}

void BPlusTree::handleLeftChild(InternalNode *previous ,LeafNode *currentLeaf,int child){

        LeafNode *leftNode = (LeafNode*) previous->children[child];

        long int i=leftNode->key_num,j=0;
        while(j<currentLeaf->key_num){
            leftNode->pointers[i]=currentLeaf->pointers[j];
            leftNode->keys[i]=currentLeaf->keys[j];
            i++;
            j++;
        }

        leftNode->key_num=leftNode->key_num+currentLeaf->key_num;
        leftNode->next_leaf=currentLeaf;
        DeleteInternalNode(previous->keys[child],previous,(InternalNode*) currentLeaf);
        DeleteNode(currentLeaf);
       
}

void BPlusTree:: handleRightChild(InternalNode *previous, LeafNode *currentLeaf,int child){

        LeafNode *rightNode = (LeafNode*) previous->children[child];

        long int i=currentLeaf->key_num,j=0;
        while(j<rightNode->key_num){
            currentLeaf->pointers[i]=rightNode->pointers[j];
            currentLeaf->keys[i]=rightNode->keys[j];
            i++;
            j++;
        }

        currentLeaf->key_num=currentLeaf->key_num+rightNode->key_num;
        currentLeaf->next_leaf=rightNode;
        DeleteInternalNode(previous->keys[child-1],previous,(InternalNode*)rightNode);
        DeleteNode(rightNode);
}

void BPlusTree:: DeleteInternalNode(const KeyType &key, InternalNode *previous, InternalNode *current){
    if(previous==(InternalNode*) root){

        if(previous->key_num==1){

            if(previous->children[1]==current){
                root=(Node*) previous->children[0];
                return;
            }

            else if(previous->children[0]==current){
                root=(Node*) previous->children[1];
                return;
            }
        }
    }
    long int position=findRemoveNode(key,(LeafNode*)previous);
    long int i=0;

    i=position;
    while(i<previous->key_num){
        previous->keys[i]=previous->keys[i+1];
        i++;

    }

    position=0;
    while(position<previous->key_num+1){
        if(previous->children[position]==current)
            break;
        position++;
    }
 
    i=position;
    while(i<previous->key_num){
        previous->children[i]=previous->children[i+1];
        i++;

    }

    previous->key_num--;

    if(previous->key_num>=(MAX_FANOUT)/2-1)
        return;

    if(previous==(InternalNode*)root)
        return;

    InternalNode *parentNode = findParent(root,previous);
    long int child[2];

    position=0;
    while(position<parentNode->key_num+1){
         if(parentNode->children[position]==previous){
            child[0]=position-1;
            child[1]=position+1;
            break;
        }
        position++;
    }

    if(child[0]>=0){
        InternalNode* leftNode=(InternalNode*) parentNode->children[child[0]];
        if(leftNode->key_num>=(MAX_FANOUT)/2){

            long int i=previous->key_num;
            while(i>0){
                previous->keys[i]=previous->keys[i-1];
                i--;
            }

            previous->keys[0]=parentNode->keys[child[0]];
            parentNode->keys[child[0]]=leftNode->keys[leftNode->key_num-1];

            i=previous->key_num+1;
            while(i>0){
                previous->children[i]=previous->children[i-1];
                i--;
            }

            previous->children[0]=leftNode->children[leftNode->key_num];
            previous->key_num++;
            leftNode->key_num--;
            return;
        }
    }
    if(child[1]<=parentNode->key_num){
        InternalNode *rightNode = (InternalNode*) parentNode->children[child[1]];
        if(rightNode->key_num>=(MAX_FANOUT)/2){

            previous->keys[previous->key_num]=parentNode->keys[position];
            parentNode->keys[position]=rightNode->keys[0];

            long int i=0;
            while(i<rightNode->key_num-1){
                rightNode->keys[i]=rightNode->keys[i+1];
                i++;
            }

            previous->children[previous->key_num+1] =rightNode->children[0];

            i=0;
            while(i>rightNode->key_num){
                rightNode->children[i]=rightNode->children[i+1];
                i++;
            }

            previous->key_num++;
            rightNode->key_num--;
            return;
        }
    }

    if(child[0]>=0){
        handleLeftChildInternal(parentNode,previous, child[0]);
    }

    else if(child[1]<=parentNode->key_num){
        handleRightChildInternal(parentNode,previous,child[1]);
    }
}

void BPlusTree:: handleLeftChildInternal(InternalNode *parentNode, InternalNode *previous, int child ){

        InternalNode* leftNode=(InternalNode*) parentNode->children[child];
        leftNode->keys[leftNode->key_num]=parentNode->keys[child];
        
        long int i=leftNode->key_num,j=0;
        while(j<previous->key_num){
            leftNode->keys[i]=previous->keys[j];
            j++;
        }

        i=leftNode->key_num+1;
        j=0;
        while(j<previous->key_num+1){
            leftNode->children[i]=previous->children[j];
            previous->children[j]=NULL;
            j++;
        }

        leftNode->key_num=leftNode->key_num+previous->key_num+1;
        previous->key_num=0;
        DeleteInternalNode(parentNode->keys[child],parentNode,previous);
}

void BPlusTree:: handleRightChildInternal(InternalNode *parentNode, InternalNode *previous, int child){
        
        InternalNode *rightNode=(InternalNode*) parentNode->children[child];
        previous->keys[previous->key_num]=parentNode->keys[child];
        
        long int i=previous->key_num+1,j=0;
        while(j<rightNode->key_num){
            previous->keys[i]=rightNode->keys[j];
            j++;
        }

        i=previous->key_num+1;
        j=0;
        while(j<rightNode->key_num){
            previous->children[i]=rightNode->children[j];
            rightNode->children[j]=NULL;
            j++;
        }

        previous->key_num=previous->key_num+rightNode->key_num+1;
        rightNode->key_num=0;
        DeleteInternalNode(parentNode->keys[child-1],parentNode,rightNode);
}

void BPlusTree:: DeleteNode(LeafNode *node){
    node->key_num=0;
    delete node;
}

int BPlusTree:: findRemoveNode(const KeyType &key,LeafNode *currentLeaf){
    long int position=0;
     while(position<currentLeaf->key_num){
        if(currentLeaf->keys[position] == key){
            break;
        }
        position++;
    }
    return position;
}
/*****************************************************************************
 * RANGE_SCAN
 *****************************************************************************/
/*
 * Return the values that within the given key range
 * First find the node large or equal to the key_start, then traverse the leaf
 * nodes until meet the key_end position, fetch all the records.
 */
void BPlusTree::RangeScan(const KeyType &key_start, const KeyType &key_end, std::vector<RecordPointer> &result) {
    
    //if tree is empty, cannot perform range scan
    if(root==NULL)
        return;
    
    //if tree is not empty, we carry out the function
    InternalNode *current = (InternalNode*) root;
    
    //drilling down to find the leaf node
    while(current->is_leaf==false){

    for(int i=0;i<current->key_num;i++){

            if(key_start<current->keys[i]){
                current=(InternalNode*) current->children[i];
                break;
            }

            if(i==current->key_num-1){
                current=(InternalNode*)current->children[i+1];
                break;
            }

        }
    }

    int position;
    LeafNode *currentLeaf = (LeafNode*) current;


        for(int i=0;i<currentLeaf->key_num;i++){
            if(currentLeaf->keys[i]>=key_start){
                position=i;
                break;
            }
        }
    
    while(currentLeaf!=NULL && currentLeaf->keys[position]<=key_end){
        int j;
		for(j=position; j<currentLeaf->key_num && currentLeaf->keys[j]<=key_end; j++){
            result.push_back(currentLeaf->pointers[j]);
        }

        if(j==currentLeaf->key_num || currentLeaf->keys[j]>=key_end){
            position=0;
            currentLeaf=currentLeaf->next_leaf;
        }
    }
}

int BPlusTree:: getPosition(LeafNode *currentLeaf, const int &key_start){
    int position=0;
    for(int i=0;i<currentLeaf->key_num;i++){
        if(currentLeaf->keys[i]>=key_start){
            position=i;
             break;
        }
    }
    
    return position;
}
