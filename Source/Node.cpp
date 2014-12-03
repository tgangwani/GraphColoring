#include "graphColor.h"
#include "string"
#define CkIntbits (sizeof(int)*8)
#define CkPriobitsToInts(nBits)    ((nBits+CkIntbits-1)/CkIntbits)
extern CkGroupID counterGroup;

//#define DEBUG 

/* -----------------------------------------
 * This is the node state BEFORE the search is initiated.
 * 1. initialize node_state_ with adjList all uncolored
 * 2. Does pre-coloring
 * ---------------------------------------*/
Node::Node(bool isRoot, int n, CProxy_Node parent) :
  nodeID_("0"), parent_(parent), uncolored_num_(n), is_root_(isRoot),
  child_num_(0), child_finished_(0),
  child_succeed_(0), is_and_node_(false), parentBits(1), parentPtr(NULL)

{
  CkAssert(isRoot);  // nodeID=1, since this constructor is only called for root chare
  vertex v = vertex(chromaticNum_);
  node_state_ = std::vector<vertex>(vertices_, v);

  //preColor();
#ifdef  DEBUG 
  CkPrintf("After Precolor\n");
  printGraph();
#endif 
  
  for (AdjListType::const_iterator it = adjList_.begin(); it != adjList_.end(); ++it) {
    uncolored_num_ -= vertexRemoval((*it).first);
  } 

#ifdef  DEBUG 
  CkPrintf("Vertex Removal\n");
  printGraph();
#endif 

  CProxy_counter(counterGroup).ckLocalBranch()->registerMe();
  thisProxy.run();
}

/* --------------------------------------------
 * Node constructor with two parameters
 * used to fire child node by state configured
 * ---------------------------------------------*/
Node::Node( std::vector<vertex> state, bool isRoot, int uncol, 
    CProxy_Node parent, std::string nodeID, UShort pBits, UInt* pPtr, int size) : 
  nodeID_(nodeID), parent_(parent), uncolored_num_(uncol), node_state_(state), 
  is_root_(isRoot), child_num_(0), child_finished_(0),
    child_succeed_(0), is_and_node_(false), parentBits(pBits), parentPtr(pPtr)

{
  for (AdjListType::const_iterator it = adjList_.begin(); it != adjList_.end(); ++it) {
    uncolored_num_ -= vertexRemoval((*it).first);
  }

  CProxy_counter(counterGroup).ckLocalBranch()->registerMe();
  thisProxy.run();
}

/*  For a vertex 'vertex', return the number of nodes
 *  those are uncolored and not pushed on stack.   
 */
int Node::getUncoloredNgbr(int vertex)
{
  std::list<int> ngbr = adjList_[vertex];
  int num = 0;
  for(std::list<int>:: const_iterator it = ngbr.begin(), jt = ngbr.end(); 
      it != jt ; it++) {
    if(false == node_state_[*it].isColored() && true == node_state_[*it].isOperationPermissible())
      num ++;
  }
  return num;
}

/*  Remove uncolored vertices recursively if the number of 
 *  is possible colorings is more than its 
 *  uncolored (and undeleted ) nghbrs.
 */
int Node::vertexRemoval(int vertex)
{
  int vertexRemoved = 0;
  if(node_state_[vertex].isColored() || !node_state_[vertex].isOperationPermissible()) {
    return 0;
  }
  boost::dynamic_bitset<> possColors = node_state_[vertex].getPossibleColor();    
  int possColorCount  = possColors.count();
  int uncoloredNgbr   = getUncoloredNgbr(vertex);
  if(possColorCount > uncoloredNgbr) {
    node_state_[vertex].set_is_onStack(true);
    deletedV.push(vertex);
    vertexRemoved ++;
    std::list<int> ngbr = adjList_[vertex];
    for(std::list<int>:: const_iterator it = ngbr.begin(), jt = ngbr.end(); 
        it != jt ; it++) {
      vertexRemoved += vertexRemoval(*it);
    }
  }
  return vertexRemoved;
}

/*  Coloring clique (i.j,k).
 *  assign min possible color to i and update its ngh.
 *  Do the same for j and k.
 *  While we are doing clique coloring we are also doing 
 *  forced moves (using updateState) which will give
 *  further improvements.
 */
void Node::colorClique3(int i, int j, int k)
{
  if(false == node_state_[i].isColored()) {
    boost::dynamic_bitset<> i_possC = node_state_[i].getPossibleColor();
    size_t i_color = i_possC.find_first();
    CkAssert(i_color != boost::dynamic_bitset<>::npos);
    int verticesColored =  updateState(node_state_, i, i_color , true);
    uncolored_num_  -=  verticesColored;
  }

  if(false == node_state_[j].isColored()) {
    boost::dynamic_bitset<> j_possC = node_state_[j].getPossibleColor();
    size_t j_color = j_possC.find_first();
    CkAssert(j_color != boost::dynamic_bitset<>::npos);
    int verticesColored = updateState(node_state_, j, j_color , true);
    uncolored_num_ -= verticesColored;
  }

  if(false == node_state_[k].isColored()) {
    boost::dynamic_bitset<> k_possC = node_state_[k].getPossibleColor();
    size_t k_color = k_possC.find_first();
    CkAssert(k_color != boost::dynamic_bitset<>::npos);
    int verticesColored = updateState(node_state_, k, k_color, true );
    uncolored_num_ -= verticesColored;
  }
}

/*  Finds all the cliques of size 3
 *  for each edge (u,v):
 *    for each vertex w:
 *      if (v,w) is an edge and (w,u) is an edge:
 *        return true
 *  and colors them using a simple
 *  coloring algorithm.
 */
void Node::preColor() 
{
  for (AdjListType::const_iterator it = adjList_.begin(); it != adjList_.end(); ++it) {
    for(std::list<int>::const_iterator jt = it->second.begin(); jt != it->second.end(); jt++ ) {
      for (AdjListType::const_iterator kt = adjList_.begin(); kt != adjList_.end(); ++kt) {

        if(node_state_[(*it).first].isColored() && node_state_[(*jt)].isColored() 
            && node_state_[(*kt).first].isColored()) {
          continue;
        }
        std::list<int>:: const_iterator it_ik = std::find(it->second.begin(), it->second.end(), (*kt).first);  
        std::list<int>:: const_iterator it_kj = std::find(kt->second.begin(), kt->second.end(), (*jt));  

        if(it_ik != it->second.end() && it_kj != kt->second.end()) {
          colorClique3((*it).first, (*jt), (*kt).first);         
        }
      }
    }
  }

}


Node::Node (CkMigrateMessage*) 
{
}

/* ----------------------------------------------
 *  Return the index of next uncolored and unstacked vertex which
 *  is most constrained.
 *  This heuristic will choose a vertex with the
 *  fewest colors available to it. 
 * 
 *  Return: index of most constrained vertex which is
 *  Else -1;
 *  ----------------------------------------------
 */
int Node::getNextConstraintVertex(){

  int cVertex, cVextexPossColor; 

  cVertex = -1;
  cVextexPossColor = chromaticNum_ + 1;

  for(int  i = 0 ; i < node_state_.size(); i++){
    if(false == node_state_[i].isColored() && true == node_state_[i].isOperationPermissible()) {
      boost::dynamic_bitset<> possibleColor = node_state_[i].getPossibleColor(); 
      if(cVextexPossColor > possibleColor.count() ) {
        cVertex = i;
        cVextexPossColor = possibleColor.count() ;
      }
    }
  }

  return cVertex;
}

/* ----------------------------------------------
 *  For a vertex id 'vIndex', returns the ordering of 
 *  possible colors, i.e. c1 > c2, such that if we color vIndex with
 *  c1 then the number of possible colorings of its neighbours 
 *  will be more than the number if vIndex is colored with c2. 
 *  Also, if for a color c, the nghbr of vIndex reduced to
 *  0 possible colors, then that c will not be considered.(Impossibility Testing)
 *  ----------------------------------------------
 */
pq_type Node::getValueOrderingOfColors(int vIndex) 
{
  pq_type  priorityColors;

  boost::dynamic_bitset<> possibleColor = node_state_[vIndex].getPossibleColor();
  if(false == possibleColor.any()) {
    return priorityColors;
  }

  std::list<int> neighbours = adjList_[vIndex];

  for(boost::dynamic_bitset<>::size_type c=0; c<possibleColor.size(); c++){
    if(false == possibleColor.test(c)) {
      continue;
    }
    bool impossColoring = false;
    int rank = 0 ;

    for(std::list<int>::const_iterator jt = neighbours.begin(); jt != neighbours.end(); jt++ ) {
      if(node_state_[*jt].isColored() || !node_state_[*jt].isOperationPermissible()) continue;
      boost::dynamic_bitset<> possibleColorOfNgb  = node_state_[*jt].getPossibleColor();
      int count = possibleColorOfNgb.test(c) ? possibleColorOfNgb.count() -1 : possibleColorOfNgb.count();
      if(0 == count) {
        impossColoring = true;
        break;
      }
      rank = rank +  count;
    }
    if(false == impossColoring) {
      priorityColors.push(std::pair<size_t,int>(c,rank));
    }
  }
  return priorityColors;
}

/*---------------------------------------------
 * updates an input state
 * 1. Color vertex[vIndex] with color c
 * 2. Updates all its neighbors corresponding possible colors
 * 3. doForcedMove == true -> color the ngh nodes recursively
 *                    if their possibility is reduced to 1.
 * 4. Return the number of vertices colored in the process. 
 * --------------------------------------------*/
int Node::updateState(std::vector<vertex> & state, int vIndex, size_t c, bool doForcedMove){
  int verticesColored = 0;

  if(state[vIndex].isColored() || !state[vIndex].isOperationPermissible()){ 
    return 0;
  }

  state[vIndex].setColor(c);
  verticesColored ++;

  for(std::list<int>::iterator it=adjList_[vIndex].begin();
      it!=adjList_[vIndex].end(); it++){
    state[*it].removePossibleColor(c);
  }

  if(true == doForcedMove) {
    for(std::list<int>::iterator it=adjList_[vIndex].begin();
        it!=adjList_[vIndex].end(); it++){
      boost::dynamic_bitset<> possColor = state[(*it)].getPossibleColor();
      if(1 == possColor.count()) {
        verticesColored += updateState(state, (*it), possColor.find_first(), doForcedMove);
      }
    }
  }
  return verticesColored;
}

void Node::printStats()
{
  CProxy_counter grp(counterGroup);
  DUMMYMSG* msg = grp[0].getTotalCount();
  int totalCharesSpawned = msg->val;
  CkPrintf("Total Chares Spawned (till first solution) = %d\n", totalCharesSpawned);
  delete msg;

}
/*--------------------------------------------
 * color the remaining graph locally
 * return if finish or not
 * -----------------------------------------*/
void Node::colorLocally()
{
    //-------------debug code below---------------
#ifdef DEBUG
    char *id = new char[nodeID_.size()];
    strcpy(id, nodeID_.c_str());
    CkPrintf("Color locally in node [%s] with uncolored num=%d\n", id, uncolored_num_);
    delete [] id;
#endif
    //------------debug code above----------------
  // 'vertex removal' and/or 'forced move' helped take out all the remaining
  // vertices, and we have none for the sequential algorithm
  if(0 == uncolored_num_) {
    mergeRemovedVerticesBack(deletedV, node_state_);

    if(is_root_) {
#ifdef DEBUG
      printGraph(true);
#endif
      CkPrintf("Sequential Coloring called from root. No parallelism\n");
      CkAssert(1 == isColoringValid(node_state_));

      CkExit();
    } else {
      parent_.finish(true, node_state_);
    }
    return;
  }
 
  if(sequentialColoring()){
    mergeRemovedVerticesBack(deletedV, node_state_);

    if(is_root_){
      CkAssert(1 == isColoringValid(node_state_));
#ifdef DEBUG
      printGraph(true);
#endif

      CkExit();
    } else {
      parent_.finish(true, node_state_);
    }
  } else {
    if(is_root_){
      CkPrintf("Fail to color!\n");
      CkExit();
    } else {
      mergeRemovedVerticesBack(deletedV, node_state_);
      parent_.finish(false, node_state_);
    }
  }
}

bool Node::sequentialColoring()
{
  // stackForSequential = Stack which holds stackNodes objects. Each stackNode
  // object represents a node of the state space search. stackNode class is
  // similar to the node class
  stackForSequential.emplace(stackNode(node_state_, uncolored_num_));
  bool solutionFound = false; 
  
  // if a solution is found in the recursive sequentialColoringHelper function,
  // the vertices in node_state_ are updated (colored)
  sequentialColoringHelper(solutionFound, node_state_);
  return solutionFound;
}

void Node::sequentialColoringHelper(bool& solutionFound, std::vector<vertex>& result)
{
  CkAssert(!stackForSequential.empty());
  
  stackNode curr_node_ = stackForSequential.top();
  stackForSequential.pop();
  if(!solutionFound)
  {
    // vertex removal step
    for (AdjListType::const_iterator it = adjList_.begin(); it != adjList_.end(); ++it) {
      curr_node_.uncolored_num_ -= curr_node_.vertexRemoval((*it).first);
    }

    // coloring found, merge the vertices we removed in the vertex removal step,
    // store the result, and return
    if(curr_node_.uncolored_num_==0)
    {
      solutionFound = true;
      curr_node_.mergeRemovedVerticesBack();
      result = curr_node_.node_state_;
      return;
    }

    // get next contrained vertex, apply value ordering. The order specified by
    // the priority queue leads to LIFO DFS of the state space
    int vIndex = curr_node_.getNextConstrainedVertex();
    CkAssert(vIndex!=-1);

    pq_type priorityColors = curr_node_.getValueOrderingOfColors(vIndex);
    while(!priorityColors.empty()){
      std::pair<int,int> p = priorityColors.top();
      priorityColors.pop();
      std::vector<vertex> new_state = curr_node_.node_state_;
      int verticesColored = curr_node_.updateState(new_state, vIndex, p.first, true);

      // recursive call
      stackForSequential.emplace(stackNode(new_state, curr_node_.uncolored_num_ - verticesColored));
      sequentialColoringHelper(solutionFound, result);

      // Don't go down on the siblings if a solution was found at a node
      if(solutionFound) { 
        curr_node_.node_state_ = result;
        break;
      }
    }
    
    // merge the vertices which were removed in vertex removal step, update
    // result
    curr_node_.mergeRemovedVerticesBack();
    if(solutionFound)
      result = curr_node_.node_state_;
  }
}

/*-------------------------------------------
 * A Node coloring routine which does the following 
 *  - Find the next constrained vertex, vIndex  (variable Ordering)
 *  - Test if the `vIndex` vertex is colorable. Return false if not.
 *  - Find the colors possible for vIndex (Value Ordering of Colors)
 *    - The colors will be found in the priority order   
 *    - If for a particular color option, one of the ngb of
 *      vIndex is reduced to zero available colors, then dont consider that
 *      color. (Impossibility Testing)
 *  - Spawn a new node state in priority order for each possible  color.
 *    - In that Node state, mark the color of vIndex
 *    - Update the coloring info for ngbs of vIndex
 *    - If by coloring vIndex with a particular color
 *      one of its ngbs reduced to one color possibility,
 *      then color that vertex as well recursively. (Forced Move) 
 *   ----------------------------------------*/
void Node::colorRemotely(){

    //-------------debug code below---------------
#ifdef DEBUG
    char *id = new char[nodeID_.size()];
    strcpy(id, nodeID_.c_str());
    CkPrintf("Color remotely in node [%s] with uncolored num=%d\n", id, uncolored_num_);
    delete [] id;
#endif
    //------------debug code above----------------

  //TODO: remove vertex and all other preprocessing operations
  //place here

    //Detect Subgraphs
    //If have >=2 subgraphs, make this node and_node
    //and spawn children to color each subgraphs
     std::map<boost::dynamic_bitset<>, std::vector<vertex>> subgraphs;
     //detect subgraphs and create states correspondingly
     if(  detectAndCreateSubgraphs( subgraphs ) ){

         //----------debug code below-----------------------
#ifdef DEUBG
         char * id = new char[nodeID_.size()+1];
         strcpy(id, nodeID_.c_str());
         CkPrintf("find %d subgraphs in node[%s]\n", subgraphs.size(), id);
         delete [] id;
         for(auto subgraph_entry : subgraphs ){
             std::string s;
             boost::to_string(subgraph_entry.first, s);
             char * bits = new char[s.size()+1];
             strcpy(bits, s.c_str());
             CkPrintf("%s\n", bits);
             delete [] bits;
         }
#endif
         //----------debug code above-----------------------
        is_and_node_ = true;
        //child_num_=subgraphs.size();
        //remove vertices accordingly for each subgraph
        for(auto subgraph_entry : subgraphs ){
            //for each subgraph, fire a child node to do the work
            //parameters
            //state, isRoot, uncoloredNum, parentProxy, nodeId
            CProxy_Node::ckNew(subgraph_entry.second, false,
                    subgraph_entry.first.count(), thisProxy,
                    nodeID_+std::to_string(child_num_),
                    0, NULL, 0);
            child_num_++;
        }
        return;
     }

  // -----------------------------------------
  // Following code deals with case is_and_node=false
  // -----------------------------------------

  int vIndex = this->getNextConstraintVertex();
  CkAssert(vIndex!=-1);

  boost::dynamic_bitset<> possibleColor = node_state_[vIndex].getPossibleColor();
  if(!possibleColor.any()){
    parent_.finish(false, node_state_);
    return;
  }

  pq_type priorityColors = getValueOrderingOfColors(vIndex);
  int numChildrenStates = priorityColors.size();
  UShort childBits = _log(numChildrenStates);

  while (! priorityColors.empty()) {

    /* priorityColors contains the vertex 
    *   colors (c1 > c2 > c3 ..) in the order governed by the valueOrdering.
    *   If we are not concerned about prioritization while
    *   firing chares, i.e. doPriority == false , we can just spawn a chare for each poped
    *   element of priorityColors. Else if (doPriority == true)
    *   and say priorityColors has elements c1: c2:c3 (such that c1 > c2 > c3)
    *   then we have to fire c1, c2 and c3 with priority Ptr00, Ptr01, Ptr10
    *   respectively, where Ptr is the priority bits for their parent.
    */
    std::pair<int,int> p =  priorityColors.top();
    priorityColors.pop();

    std::vector<vertex> new_state = node_state_;
    int verticesColored = updateState(new_state, vIndex, p.first, true);

    if(doPriority) {
      CkEntryOptions* opts = new CkEntryOptions ();
      UShort newParentPrioBits; UInt* newParentPrioPtr;
      UInt newParentPrioPtrSize;
      getPriorityInfo(newParentPrioBits, newParentPrioPtr, newParentPrioPtrSize, parentBits, parentPtr, childBits, child_num_);
      opts->setPriority(newParentPrioBits, newParentPrioPtr);

      CProxy_Node::ckNew(new_state, false, uncolored_num_- verticesColored, 
          thisProxy, nodeID_ + std::to_string(child_num_), newParentPrioBits, newParentPrioPtr, 
          newParentPrioPtrSize, CK_PE_ANY , opts);
      child_num_ ++;
      free(newParentPrioPtr);
    } else {
      CProxy_Node::ckNew(new_state, false, uncolored_num_- verticesColored, 
          thisProxy, nodeID_ + std::to_string(child_num_), 0, NULL, 0);
      child_num_ ++;
    }
  }
}

void Node::getPriorityInfo(UShort & newParentPrioBits, UInt* &newParentPrioPtr, UInt &newParentPrioPtrSize, 
      UShort &parentPrioBits, UInt* &parentPrioPtr, UShort &childPrioBits, UInt &childnum)
{
  if(NULL == parentPrioPtr) {
    CkAssert(childPrioBits <= CkIntbits);
    newParentPrioBits  = childPrioBits + parentPrioBits;
    newParentPrioPtr  = (UInt *)malloc(sizeof(int));
    *newParentPrioPtr = childnum << (8*sizeof(unsigned int) - newParentPrioBits);
    newParentPrioPtrSize = 1;
    return;
  }

  newParentPrioBits           = parentPrioBits + childPrioBits;
  UShort parentPrioWords      = CkPriobitsToInts(parentPrioBits);
  UShort newParentPrioWords   = CkPriobitsToInts(parentPrioBits + childPrioBits);
  int shiftbits = 0;

  if(newParentPrioWords == parentPrioWords) {
    newParentPrioPtr  = (UInt *)malloc(parentPrioWords*sizeof(int));
    for(int i=0; i<parentPrioWords; i++) {
      newParentPrioPtr[i] = parentPrioPtr[i];
    }
    newParentPrioPtrSize  = parentPrioWords;

    if((newParentPrioBits) % (8*sizeof(unsigned int)) != 0) {
      shiftbits = 8*sizeof(unsigned int) - (newParentPrioBits)%(8*sizeof(unsigned int));
    }
    newParentPrioPtr[parentPrioWords-1] = parentPrioPtr[parentPrioWords-1] | (childnum << shiftbits);

  } else if(newParentPrioWords > parentPrioWords) {
    /* have to append a new integer */
    newParentPrioPtr  = (UInt *)malloc(newParentPrioWords*sizeof(int));
    for(int i=0; i<parentPrioWords; i++) {
      newParentPrioPtr[i] = parentPrioPtr[i];
    }
    newParentPrioPtrSize  = newParentPrioWords;

    if(parentPrioBits % (8*sizeof(unsigned int)) == 0) {
      shiftbits = sizeof(unsigned int)*8 - childPrioBits;
      newParentPrioPtr[parentPrioWords] = (childnum << shiftbits);
    } else { /*higher bits are appended to the last integer and then use anothe new integer */
      int inusebits = 8*sizeof(unsigned int) - (parentPrioBits % ( 8*sizeof(unsigned int)));
      unsigned int higherbits =  childnum >> (childPrioBits - inusebits);
      newParentPrioPtr[parentPrioWords-1] = parentPrioPtr[parentPrioWords-1] | higherbits;
      /* lower bits are stored in new integer */
      newParentPrioPtr[parentPrioWords] = childnum << (8*sizeof(unsigned int) - childPrioBits + inusebits);
    }
  }
}

/* ---------------------------------------------------
 * called when receive child finish response
 * - merge the part sent by child
 * - respond to parent
 * - decide whether to wait for more child or not
 * TODO:
 *  current implementation doesn't need to merge the graph
 *  but will be needed with later optimization
 * -------------------------------------------------*/
bool Node::mergeToParent(bool res, std::vector<vertex> state)
{

  child_finished_ ++;
  if(res==true){
    child_succeed_++;
  }

  bool success = is_and_node_ ?
    //if it's and node, success means all children succeed
    child_succeed_==child_num_ :
    //if it's or node, success means at least one child succeed
    (child_succeed_!=0);

  //if all children return, don't need to wait
  bool finish  = (child_finished_ == child_num_); 
  finish  |= (is_and_node_ ?
    //if it's and node,
    //return success, terminate if finish=num
    //return fail, terminate if one fail
    (child_succeed_!=child_finished_) :
    //if it's or node,
    //return success, terminate one success
    //return fail, terminate all finish
    (child_succeed_!=0));

  if(is_and_node_){
    //TODO: call "Merge Subgraph" here
    for(int i=0; i<vertices_; i++ ){
        //if the vertex is removed from the subgraph
        //don't need to merge back
        if(state[i].get_is_onStack()==true ||
           state[i].get_is_out_of_subgraph()==true)
            continue;
        // if the vertex has been colored before
        // check whether they are matched or not
        if(node_state_[i].isColored()){
            CkAssert(node_state_[i].getColor()==state[i].getColor());
        }
        //if the color is assigned from children node
        //assign the color to current node_state_
        node_state_[i]=state[i];
    }
    CkPrintf("success=%d, finish=%d\n", success, finish);
    printGraph();
  }

  if(is_root_ && finish){
    if(success){
      if(!is_and_node_)
        node_state_ = state;
      mergeRemovedVerticesBack(deletedV, node_state_);
      printGraph();
      CkAssert(1 == isColoringValid(node_state_));
#ifdef DEBUG
      printGraph(true);
#endif


      CkExit();
    } else {
      CkPrintf("Fail to color!\n");
      CkExit();
    }
  } else if(!is_root_ && finish){
    // In one child, successfully colored
    // TODO:: once it succeeds, it should notify other child chares
    // sharing the same parent to stop working
    if(!is_and_node_)
        node_state_ = state;
    mergeRemovedVerticesBack(deletedV, node_state_);
    parent_.finish(success, node_state_);
  } 

  return !finish;
}

void Node::mergeRemovedVerticesBack(std::stack<int> deletedV, std::vector<vertex> &node_state_) {
  while(!deletedV.empty()) {
    int vertex = deletedV.top();
    deletedV.pop();
    node_state_[vertex].set_is_onStack(false);
    boost::dynamic_bitset<> possColor = node_state_[vertex].getPossibleColor();
    size_t c = possColor.find_first();
    CkAssert(c != boost::dynamic_bitset<>::npos);
#ifdef DEBUG
    CkPrintf("Popped vertex %d color %d\n", vertex,c);
#endif
    updateState(node_state_, vertex, c, false);
  }
}

/* --------------------------------------------
 * print out node_state_
 * -------------------------------------------*/
void Node::printGraph(bool final){
  if(final)
    CkPrintf("--Printing final graph %s--\n", is_root_?"from Root Chare":"from Non-root Chare");
  else
    CkPrintf("--Printing partial graph %s--\n", is_root_?"from Root Chare":"from Non-root Chare");

  for(int i=0; i<node_state_.size(); i++){
    CkPrintf("vertex[%d]:color[%d] ;\n", i, node_state_[i].getColor());
  }
  CkPrintf("\n-------------------------------\n");
}

/* --------------------------------------------
 * Checks if the reported coloring is valid.
 * -------------------------------------------*/
bool Node::isColoringValid(std::vector<vertex> state)
{
  // Only the root chare should check validity of solution
  CkAssert(is_root_); 

  for (AdjListType::const_iterator it = adjList_.begin(); it != adjList_.end(); ++it) {

    int iColor = state[(*it).first].getColor();

    if(iColor >= chromaticNum_ || iColor == -1 ) return 0; 

    for(std::list<int>::const_iterator jt = it->second.begin(); jt != it->second.end(); jt++ ) {
      int jColor = state[*jt].getColor();
      if(jColor >= chromaticNum_ || -1 == jColor || iColor == jColor) return 0; 
    }

  }  

  // valid coloring! Print statistics (if any)
  CkPrintf("Valid Coloring found. Hurray! Printing stats.\n");
  int vertexRemovalEfficiency = 0;
  for_each(state.begin(), state.end(), [&](vertex v){
      if(v._stat_vertexRemoval)
        vertexRemovalEfficiency++;
      });
  
  CkPrintf("Vertices removed by [Vertex Removal] = %d\n", vertexRemovalEfficiency);
  printStats();
  return  1;
}

int Node::_log(int n)
{
  int _mylog = 0;
  for(n=n-1;n>0;n=n>>1)
  {
    _mylog++;
  }
  return _mylog;
}


//--------------------------------------------------------
//detectAndCreateSubgraphs
// - return true if it consists more than one subgraphs
//---------------------------------------------------------
bool Node::detectAndCreateSubgraphs(
        std::map<boost::dynamic_bitset<>, std::vector<vertex>> & subgraphs)
{
    boost::dynamic_bitset<> init_bitset(vertices_);
    init_bitset.set();
    //initialize the bitset by marking all removed vertices as 0
    //and existed vertices as 1
    CkAssert(node_state_.size()==vertices_);
    for(int i=0; i<vertices_; i++){
       //these vertices have been removed from current graph
       if(!node_state_[i].isOperationPermissible())
            init_bitset.reset(i);
    }
   // #ifdef DEBUG
        if(init_bitset.count()!=vertices_){
        std::string s;
         char * id = new char[nodeID_.size()+1];
         strcpy(id, nodeID_.c_str());
             boost::to_string(init_bitset, s);
             char * bits = new char[s.size()+1];
             strcpy(bits, s.c_str());
             CkPrintf("in node [%s] detect subgraphs %s\n", id, bits);
             delete [] id;
             delete [] bits;
        }
   // #endif
    //keep track of vertices haven't been assigned to any subgraph
    boost::dynamic_bitset<> work_bitset(init_bitset);
    //record vertices in current computing subgraph
    boost::dynamic_bitset<> subgraph_bitset(vertices_);
    std::list<int> worklist;

    while(work_bitset.any()){
        //start working on getting a new subgraph
        subgraph_bitset.reset();
        worklist.clear();
        //get an unremoved and haven't considered vertex
        int first_bit = 0;
        while(!work_bitset.test(first_bit))
            first_bit++;
        //add it to current subgraph
        subgraph_bitset.set(first_bit);
        work_bitset.reset(first_bit);
        worklist.push_back(first_bit);

        do{
            //add its neighbors to subgraph iteratively
            int i = worklist.front();
            worklist.pop_front();
            for( int neighbor_vertex_index : adjList_[i]){
                //if the neighbor vertex exists and haven't considered
                //put it into current subgraph
                if(work_bitset.test(neighbor_vertex_index)){
                    subgraph_bitset.set(neighbor_vertex_index);
                    work_bitset.reset(neighbor_vertex_index);
                    worklist.push_back(neighbor_vertex_index);
                }
            }
        }while(!worklist.empty());

        //finish getting one subgraph
        //create corresponding states and insert <bitset, state> to the map
        std::vector<vertex> new_state = node_state_;
        //for this status different from initial states
        //we have to mark them as out_of_subgraph
        boost::dynamic_bitset<> remove_bitset = init_bitset ^ subgraph_bitset;
        for(int i=0; i<remove_bitset.size(); i++){
            if(remove_bitset.test(i)){
                //remove from the list
                new_state[i].set_out_of_subgraph(true);
            }
        }
        subgraphs.insert(std::pair<boost::dynamic_bitset<>, std::vector<vertex>>(
                    subgraph_bitset, new_state));
    }

    //return true, if exists more than 2 subgraphs
    return subgraphs.size()>1  ;
}
