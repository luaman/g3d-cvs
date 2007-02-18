/**
  @file PointAABSPTree.h
  
  @maintainer Morgan McGuire, matrix@graphics3d.com
 
  @created 2004-01-11
  @edited  2007-02-12

  Copyright 2000-2007, Morgan McGuire.
  All rights reserved.
  
  */

#ifndef G3D_PointAABSPTree_H
#define G3D_PointAABSPTree_H

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"
#include "G3D/AABox.h"
#include "G3D/Sphere.h"
#include "G3D/Box.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/CollisionDetection.h"
#include <algorithm>

inline void getPosition(const G3D::Vector3& v, G3D::Vector3& p) {
    p = v;
}

inline void getPosition(const G3D::Vector4& v,  G3D::Vector3& p) {
    p = v.xyz();
}

inline void getPosition(const G3D::Vector2& v, G3D::Vector3& p) {
    p.x = v.x;
    p.y = v.y;
    p.z = 0;
}

namespace G3D {

/**
 A set data structure that supports spatial queries using an axis-aligned
 BSP tree for speed.

 PointAABSPTree allows you to quickly find points in 3D that lie within
 a box or sphere. For large sets of objects it is much faster
 than testing each object for a collision.  See also G3D::AABSPTree; this class
 is optimized for point sets, e.g.,for use in photon mapping and mesh processing.

 <B>Template Parameters</B>

 <br>

 <br>The template parameter <I>T</I> must be one for which
 the following functions are overloaded:

  <DT><CODE>bool ::operator==(const T&, const T&);</CODE>
  <DT><CODE>unsigned int ::hashCode(const T&);</CODE>
  <DT><CODE>T::T();</CODE> <I>(public constructor of no arguments)</I>

 If template parameter useReferences is <code>true</code>, then
  <P><CODE>const Vector3& ::getPosition(const T&);</CODE>
<p>
 must be overloaded. Otherwise
  <P><CODE>void ::getPosition(const T&, G3D::Vector3&);</CODE>
<p>
 must be overloaded.
 <p>

 G3D provides these for the Vector2, Vector3, and Vector4 classes.
 If you use a custom class, or a pointer to a custom class, you will need
 to define those functions.

 <B>Moving %Set Members</B>
 <DT>It is important that objects do not move without updating the
 PointAABSPTree.  If the position of an object is about
 to change, PointAABSPTree::remove it before they change and 
 PointAABSPTree::insert it again afterward.  For objects
 where the hashCode and == operator are invariant with respect 
 to the 3D position,
 you can use the PointAABSPTree::update method as a shortcut to
 insert/remove an object in one step after it has moved.
 

 Note: Do not mutate any value once it has been inserted into PointAABSPTree. Values
 are copied interally. All PointAABSPTree iterators convert to pointers to constant
 values to reinforce this.

 If you want to mutate the objects you intend to store in a PointAABSPTree
 simply insert <I>pointers</I> to your objects instead of the objects
 themselves, and ensure that the above operations are defined. (And
 actually, because values are copied, if your values are large you may
 want to insert pointers anyway, to save space and make the balance
 operation faster.)

 <B>Dimensions</B>
 Although designed as a 3D-data structure, you can use the PointAABSPTree
 for data distributed along 2 or 1 axes by simply returning bounds
 that are always zero along one or more dimensions.

*/
template<class T> class PointAABSPTree {
protected:

    // Unlike the AABSPTree, the PointAABSPTree assumes that T elements are
    // small and keeps the handle and cached position together instead of
    // placing them in separate bounds arrays.  Also note that a copy of T
    // is kept in the member table and that there is no indirection.
    class Handle {
    private:
        Vector3             m_position;

    public:
        T                   value;

        Handle() {}
        Handle(const T& v) : value(v), m_position(getPosition(v)) {}

        inline const Vector3& position() const {
            return m_position;
        }

        inline Handle(const T& v) : value(v) {
            getPosition(v, m_position);
        }
    };

    /** Returns the bounds of the sub array. Used by makeNode. */
    static AABox computeBounds(
        const Array<Handle>&  point, 
        int                   beginIndex,
        int                   endIndex) {
    
        if (point.size() == 0) {
            return AABox(Vector3::inf(), Vector3::inf());
        }

        Vector3 lo = point[0].position();
        Vector3 hi = lo;

        for (int p = beginIndex; p <= endIndex; ++p) {
            const Vector3& pt = point[i].position();
            lo = lo.min(pt);
            hi = hi.max(pt);
        }

        return AABox(lo, hi);
    }


    /**
     A sort predicate that returns true if the 
     first argument is less than the midpoint of the second
     along the specified axis.

     Used by makeNode.
     */
    class CenterLT {
    public:
        Vector3::Axis           sortAxis;

        CenterLT(Vector3::Axis a) : sortAxis(a) {}

        inline bool operator()(const Handle& a, const Handle& b) const {
            return a.position()[sortAxis] < b.position()[sortAxis];
        }
    };



    class Node {
    public:

        /** Spatial bounds on all values at this node and its children, based purely on
            the parent's splitting planes.  May be infinite */
        AABox               splitBounds;

        Vector3::Axis       splitAxis;

        /** Location along the specified axis */
        float               splitLocation;
 
        /** child[0] contains all values strictly 
            smaller than splitLocation along splitAxis.

            child[1] contains all values strictly
            larger.

            Both may be NULL if there are not enough
            values to bother recursing.
        */
        Node*               child[2];

        /** Values if  this is a leaf node). */
        Array<Handle>       valueArray;

        /** Creates node with NULL children */
        Node() {
            splitAxis     = Vector3::X_AXIS;
            splitLocation = 0;
            splitBounds   = AABox(-Vector3::inf(), Vector3::inf());
            for (int i = 0; i < 2; ++i) {
                child[i] = NULL;
            }
        }

        /**
         Doesn't clone children.
         */
        Node(const Node& other) : valueArray(other.valueArray) {
            splitAxis       = other.splitAxis;
            splitLocation   = other.splitLocation;
            splitBounds     = other.splitBounds;            
            for (int i = 0; i < 2; ++i) {
                child[i] = NULL;
            }
        }

        /** Copies the specified subarray of pt into point, NULLs the children.
            Assumes a second pass will set splitBounds. */
        Node(const Array<Handle>& pt, int beginIndex, int endIndex) {
            splitAxis     = Vector3::X_AXIS;
            splitLocation = 0;
            for (int i = 0; i < 2; ++i) {
                child[i] = NULL;
            }

            int n = endIndex - beginIndex + 1;

            valueArray.resize(n);
            for (int i = n - 1; i >= 0; --i) {
                valueArray[i] = pt[i + beginIndex];
            }
        }


        /** Deletes the children (but not the values) */
        ~Node() {
            for (int i = 0; i < 2; ++i) {
                delete child[i];
            }
        }


        /** Returns true if this node is a leaf (no children) */
        inline bool isLeaf() const {
            return (child[0] == NULL) && (child[1] == NULL);
        }


        /**
         Recursively appends all handles and children's handles
         to the array.
         */
        void getHandles(Array<Handle>& handleArray) const {
            handleArray.append(valueArray);
            for (int i = 0; i < 2; ++i) {
                if (child[i] != NULL) {
                    child[i]->getHandles(handleArray);
                }
            }
        }


	    void verifyNode(const Vector3& lo, const Vector3& hi) {
            //		debugPrintf("Verifying: split %d @ %f [%f, %f, %f], [%f, %f, %f]\n",
            //			    splitAxis, splitLocation, lo.x, lo.y, lo.z, hi.x, hi.y, hi.z);

            debugAssert(lo == splitBounds.low());
            debugAssert(hi == splitBounds.high());

		    for (int i = 0; i < valueArray.length(); ++i) {
			    const AABox& b = valueArray[i].bounds;

			    for(int axis = 0; axis < 3; ++axis) {
				    debugAssert(b.low()[axis] <= b.high()[axis]);
				    debugAssert(b.low()[axis] > lo[axis]);
				    debugAssert(b.high()[axis] < hi[axis]);
			    }
		    }

		    if (child[0] || child[1]) {
			    debugAssert(lo[splitAxis] < splitLocation);
			    debugAssert(hi[splitAxis] > splitLocation);
		    }

		    Vector3 newLo = lo;
		    newLo[splitAxis] = splitLocation;
		    Vector3 newHi = hi;
		    newHi[splitAxis] = splitLocation;

		    if (child[0] != NULL) {
			    child[0]->verifyNode(lo, newHi);
		    }

		    if (child[1] != NULL) {
			    child[1]->verifyNode(newLo, hi);
		    }
	    }


        /**
          Stores the locations of the splitting planes (the structure but not the content)
          so that the tree can be quickly rebuilt from a previous configuration without 
          calling balance.
         */
        static void serializeStructure(const Node* n, BinaryOutput& bo) {
            if (n == NULL) {
                bo.writeUInt8(0);
            } else {
                bo.writeUInt8(1);
                n->splitBounds.serialize(bo);
                serialize(n->splitAxis, bo);
                bo.writeFloat32(n->splitLocation);
                for (int c = 0; c < 2; ++c) {
                    serializeStructure(n->child[c], bo);
                }
            }
        }

        /** Clears the member table */
        static Node* deserializeStructure(BinaryInput& bi) {
            if (bi.readUInt8() == 0) {
                return NULL;
            } else {
                Node* n = new Node();
                n->splitBounds.deserialize(bi);
                deserialize(n->splitAxis, bi);
                n->splitLocation = bi.readFloat32();
                for (int c = 0; c < 2; ++c) {
                    n->child[c] = deserializeStructure(bi);
                }
            }
        }

        /** Returns the deepest node that completely contains bounds. */
        Node* findDeepestContainingNode(const AABox& bounds) {

            // See which side of the splitting plane the bounds are on
            if (bounds.high()[splitAxis] < splitLocation) {
                // Bounds are on the low side.  Recurse into the child
                // if it exists.
                if (child[0] != NULL) {
                    return child[0]->findDeepestContainingNode(bounds);
                }
            } else if (bounds.low()[splitAxis] > splitLocation) {
                // Bounds are on the high side, recurse into the child
                // if it exists.
                if (child[1] != NULL) {
                    return child[1]->findDeepestContainingNode(bounds);
                }
            }

            // There was no containing child, so this node is the
            // deepest containing node.
            return this;
        }


        /** Appends all members that intersect the box. 
            If useSphere is true, members are tested against the sphere instead. */
        void getIntersectingMembers(
            const AABox&        box, 
            const Sphere&       sphere,
            Array<T>&           members,
            bool                useSphere) const {

            // Test all values at this node
            for (int v = 0; v < valueArray.size(); ++v) {
                if ((useSphere && sphere.contains(valueArray[v].position())) ||
                    (! useSphere && box.contains(valueArray[v].position()))) {
                    members.append(valueArray[v].value);
                }
            }

            // If the left child overlaps the box, recurse into it
            if ((child[0] != NULL) && (box.low()[splitAxis] < splitLocation)) {
                child[0]->getIntersectingMembers(box, sphere, members, useSphere);
            }

            // If the right child overlaps the box, recurse into it
            if ((child[1] != NULL) && (box.high()[splitAxis] > splitLocation)) {
                child[1]->getIntersectingMembers(box, sphere, members, useSphere);
            }
        }

        /**
         Recurse through the tree, assigning splitBounds fields.
         */
        void assignSplitBounds(const AABox& myBounds) {
            splitBounds = myBounds;

            AABox childBounds[2];
            myBounds.split(splitAxis, splitLocation, childBounds[0], childBounds[1]);

            for (int c = 0; c < 2; ++c) {
                if (child[c]) {
                    child[c]->assignSplitBounds(childBounds[c]);
                }
            }
        }
    };
    /**
     Recursively subdivides the subarray.
     Begin and end indices are inclusive.

     Call assignSplitBounds() on the root node after making a tree.
     */
    Node* makeNode(Array<Handle>& point, int beginIndex, 
                   int endIndex, int valuesPerNode, 
                   int numMeanSplits)  {

	    Node* node = NULL;
	    
	    if (endIndex - beginIndex + 1 <= valuesPerNode) {
		    // Make a new leaf node
		    node = new Node(point, beginIndex, endIndex);
		    
		    // Set the pointers in the memberTable
		    for (int i = beginIndex; i <= endIndex; ++i) {
			    memberTable.set(point[i].value, node);
		    }
		    
        } else {
		    // Make a new internal node
		    node = new Node();
		    
            const AABox bounds = computeBounds(point, beginIndex, endIndex);
		    const Vector3 extent = bounds.high() - bounds.low();
		    
		    Vector3::Axis splitAxis = extent.primaryAxis();
		    
            double splitLocation;

	        // Sort only the subarray
	        std::sort(
		        point.getCArray() + beginIndex,
		        point.getCArray() + endIndex + 1,
		        CenterLT(splitAxis));

            if (numMeanSplits > 0) {
                // Compute the mean along the axis

                splitLocation = (bounds.high()[splitAxis] + 
                                 bounds.low()[splitAxis]) / 2.0;

            } else {

                // Compute the median along the axis
		        		        
                // Intentional integer divide
		        int midIndex = (beginIndex + endIndex) / 2;
		        
		        // Choose the split location between the two middle elements
		        const Vector3 median = 
			        (point[midIndex].position() +
			         point[iMin(midIndex + 1, point.size() - 1)].position()) * 0.5;

                splitLocation = median[splitAxis];
            }
		    
		    node->splitAxis     = splitAxis;
		    node->splitLocation = splitLocation;

            // Find the index separating those below and above the split.
            // Points on the split go below
            int splitIndex;
            for (splitIndex = beginIndex;
                 (splitIndex <= endIndex) &&
                 (point[splitIndex].position()[splitAxis] <= splitLocation); 
                 ++splitIndex) {}
		    
            // There must always be a left child, since the splitLocation cannot be lower than the mean or the median
		    node->child[0]      = 
			    makeNode(point, beginIndex, splitIndex - 1, 
                         valuesPerNode, numMeanSplits - 1);
		    
		    if (splitIndex <= endIndex) {
			    node->child[1]      = 
				    makeNode(point, splitIndex, endIndex, valuesPerNode, numMeanSplits - 1);
		    }
		    
	    }
	    
	    return node;
    }

    /**
     Recursively clone the passed in node tree, setting
     pointers for members in the memberTable as appropriate.
     called by the assignment operator.
     */
    Node* cloneTree(Node* src) {
        Node* dst = new Node(*src);

        // Make back pointers
        for (int i = 0; i < dst->valueArray.size(); ++i) {
            memberTable.set(dst->valueArray[i].value, dst);
        }

        // Clone children
        for (int i = 0; i < 2; ++i) {
            if (src->child[i] != NULL) {
                dst->child[i] = cloneTree(src->child[i]);
            }
        }

        return dst;
    }

    /** Maps members to the node containing them */
    Table<T, Node*>         memberTable;

    Node*                   root;

public:

    /** To construct a balanced tree, insert the elements and then call
      PointAABSPTree::balance(). */
    PointAABSPTree() : root(NULL) {}


    PointAABSPTree(const PointAABSPTree& src) : root(NULL) {
        *this = src;
    }


    PointAABSPTree& operator=(const PointAABSPTree& src) {
        delete root;
        // Clone tree takes care of filling out the memberTable.
        root = cloneTree(src.root);
    }


    ~PointAABSPTree() {
        clear();
    }

    /**
     Throws out all elements of the set.
     */
    void clear() {
        memberTable.clear();
        delete root;
        root = NULL;
    }

    int size() const {
        return memberTable.size();
    }

    /**
     Inserts an object into the set if it is not
     already present.  O(log n) time.  Does not
     cause the tree to be balanced.
     */
    void insert(const T& value) {
        if (contains(value)) {
            // Already in the set
            return;
        }

        Handle h(value);

        if (root == NULL) {
            // This is the first node; create a root node
            root = new Node();
        }

        Node* node = root->findDeepestContainingNode(h.position());

        // Insert into the node
        node->valueArray.append(h);
        
        // Insert into the node table
        memberTable.set(value, node);
    }

    /** Inserts each elements in the array in turn.  If the tree
        begins empty (no structure and no elements), this is faster
        than inserting each element in turn.  You still need to balance
        the tree at the end.*/
    void insert(const Array<T>& valueArray) {
        if (root == NULL) {
            // Optimized case for an empty tree; don't bother
            // searching or reallocating the root node's valueArray
            // as we incrementally insert.
            root = new Node();
            root->valueArray.resize(valueArray.size());
            for (int i = 0; i < valueArray.size(); ++i) {
                // Insert in opposite order so that we have the exact same
                // data structure as if we inserted each (i.e., order is reversed
                // from array).
                root->valueArray[valueArray.size() - i - 1] = Handle(valueArray[i]);
                memberTable.set(valueArray[i], root);
            }
        } else {
            // Insert at appropriate tree depth.
            for (int i = 0; i < valueArray.size(); ++i) {
                insert(valueArray[i]);
            }
        }
    }


    /**
     Returns true if this object is in the set, otherwise
     returns false.  O(1) time.
     */
    bool contains(const T& value) {
        return memberTable.containsKey(value);
    }


    /**
     Removes an object from the set in O(1) time.
     It is an error to remove members that are not already
     present.  May unbalance the tree.  
     
     Removing an element never causes a node (split plane) to be removed...
     nodes are only changed when the tree is rebalanced.  This behavior
     is desirable because it allows the split planes to be serialized,
     and then deserialized into an empty tree which can be repopulated.
    */
    void remove(const T& value) {
        debugAssertM(contains(value),
            "Tried to remove an element from a "
            "PointAABSPTree that was not present");

        Array<Handle>& list = memberTable[value]->valueArray;

        // Find the element and remove it
        for (int i = list.length() - 1; i >= 0; --i) {
            if (list[i].value == value) {
                list.fastRemove(i);
                break;
            }
        }
        memberTable.remove(value);
    }


    /**
     If the element is in the set, it is removed.
     The element is then inserted.

     This is useful when the == and hashCode methods
     on <I>T</I> are independent of the bounds.  In
     that case, you may call update(v) to insert an
     element for the first time and call update(v)
     again every time it moves to keep the tree 
     up to date.
     */
    void update(const T& value) {
        if (contains(value)) {
            remove(value);
        }
        insert(value);
    }


    /**
     Rebalances the tree (slow).  Call when objects
     have moved substantially from their original positions
     (which unbalances the tree and causes the spatial
     queries to be slow).
     
     @param valuesPerNode Maximum number of elements to put at
     a node. 

     @param numMeanSplits numMeanSplits = 0 gives a 
     fully axis aligned BSP-tree, where the balance operation attempts to balance
     the tree so that every splitting plane has an equal number of left
     and right children (i.e. it is a <B>median</B> split along that axis).  
     This tends to maximize average performance; all querries will return in the same amount of time.

     You can override this behavior by
     setting a number of <B>mean</B> (average) splits.  numMeanSplits = MAX_INT
     creates a full oct-tree, which tends to optimize peak performance (some areas of the scene will terminate after few recursive splits) at the expense of
     peak performance. 
     */
    void balance(int valuesPerNode = 20, int numMeanSplits = 3) {
        if (root == NULL) {
            // Tree is empty
            return;
        }

        Array<Handle> handleArray;
        root->getHandles(handleArray);

        // Delete the old tree
        clear();

        root = makeNode(handleArray, 0, handleArray.size() - 1, 
            valuesPerNode, numMeanSplits);

        // Walk the tree, assigning splitBounds.  We start with unbounded
        // space.
        root->assignSplitBounds(AABox::maxFinite());

        #ifdef _DEBUG
        root->verifyNode(Vector3::minFinite(), Vector3::maxFinite());
        #endif
    }

private:

    /**
     Returns the elements

     @param parentMask The mask that this node returned from culledBy.
     */
    static void getIntersectingMembers(
        const Array<Plane>&         plane,
        Array<T>&                   members,
        Node*                       node,
        uint32                      parentMask) {

        int dummy;

        if (parentMask == 0) {
            // None of these planes can cull anything
            for (int v = node->valueArray.size() - 1; v >= 0; --v) {
                members.append(node->valueArray[v].value);
            }

            // Iterate through child nodes
            for (int c = 0; c < 2; ++c) {
                if (node->child[c]) {
                    getIntersectingMembers(plane, members, node->child[c], 0);
                }
            }
        } else {

            if (node->valueArray.size() >  0) {
                // This is a leaf; check the points
                debugAssertM(node->child[0] == NULL, "Malformed Point tree");
                debugAssertM(node->child[1] == NULL, "Malformed Point tree");

                // Test values at this node against remaining planes
                for (int p = 0; p < plane.size(); ++p) {
                    if ((parentMask >> p) & 1 != 0) {
                        // Test against this plane
                        const Plane& curPlane = plane[p];
                        for (int v = node->valueArray.size() - 1; v >= 0; --v) {
                            if (curPlane.halfSpaceContains(node->valueArray[v].position())) {
                                members.append(node->valueArray[v].value);
                            }
                        }
                    }
                }
            } else {

                uint32 childMask  = 0xFFFFFF;

                // Iterate through child nodes
                for (int c = 0; c < 2; ++c) {
                    if (node->child[c] &&
                        ! node->child[c]->splitBounds.culledBy(plane, dummy, parentMask, childMask)) {
                        // This node was not culled
                        getIntersectingMembers(plane, members, node->child[c], childMask);
                    }
                }
            }
        }
    }

public:

    /**
     Returns all members inside the set of planes.  
      @param members The results are appended to this array.
     */
    void getIntersectingMembers(const Array<Plane>& plane, Array<T>& members) const {
        if (root == NULL) {
            return;
        }

        getIntersectingMembers(plane, members, root, 0xFFFFFF);
    }

    /**
     Typically used to find all visible
     objects inside the view frustum (see also GCamera::getClipPlanes)... i.e. all objects
     <B>not<B> culled by frustum.

     Example:
      <PRE>
        Array<Object*>  visible;
        tree.getIntersectingMembers(camera.frustum(), visible);
        // ... Draw all objects in the visible array.
      </PRE>
      @param members The results are appended to this array.
      */
    void getIntersectingMembers(const GCamera::Frustum& frustum, Array<T>& members) const {
        Array<Plane> plane;
        
        for (int i = 0; i < frustum.faceArray.size(); ++i) {
            plane.append(frustum.faceArray[i].plane);
        }

        getIntersectingMembers(plane, members);
    }

    /**
     C++ STL style iterator variable.  See beginBoxIntersection().
     The iterator overloads the -> (dereference) operator, so this
     acts like a pointer to the current member.
     */
    // This iterator turns Node::getIntersectingMembers into a
    // coroutine.  It first translates that method from recursive to
    // stack based, then captures the system state (analogous to a Scheme
    // continuation) after each element is appended to the member array,
    // and allowing the computation to be restarted.
    class BoxIntersectionIterator {
    private:
        friend class PointAABSPTree<T>;

        /** True if this is the "end" iterator instance */
        bool            isEnd;

        /** The box that we're testing against. */
        AABox           box;

        /** Node that we're currently looking at.  Undefined if isEnd
            is true. */
        Node*           node;

        /** Nodes waiting to be processed */
        // We could use backpointers within the tree and careful
        // state management to avoid ever storing the stack-- but
        // it is much easier this way and only inefficient if the
        // caller uses post increment (which they shouldn't!).
        Array<Node*>    stack;

        /** The next index of current->valueArray to return. 
            Undefined when isEnd is true.*/
        int             nextValueArrayIndex;

        BoxIntersectionIterator() : isEnd(true) {}
        
        BoxIntersectionIterator(const AABox& b, const Node* root) : 
           isEnd(root == NULL), box(b), 
           node(const_cast<Node*>(root)), nextValueArrayIndex(-1) {

           // We intentionally start at the "-1" index of the current
           // node so we can use the preincrement operator to move
           // ourselves to element 0 instead of repeating all of the
           // code from the preincrement method.  Note that this might
           // cause us to become the "end" instance.
           ++(*this);
        }

    public:

        inline bool operator!=(const BoxIntersectionIterator& other) const {
            return ! (*this == other);
        }

        bool operator==(const BoxIntersectionIterator& other) const {
            if (isEnd) {
                return other.isEnd;
            } else if (other.isEnd) {
                return false;
            } else {
                // Two non-end iterators; see if they match.  This is kind of 
                // silly; users shouldn't call == on iterators in general unless
                // one of them is the end iterator.
                if ((box != other.box) || (node != other.node) || 
                    (nextValueArrayIndex != other.nextValueArrayIndex) ||
                    (stack.length() != other.stack.length())) {
                    return false;
                }

                // See if the stacks are the same
                for (int i = 0; i < stack.length(); ++i) {
                    if (stack[i] != other.stack[i]) {
                        return false;
                    }
                }

                // We failed to find a difference; they must be the same
                return true;
            }
        }

        /**
         Pre increment.
         */
        BoxIntersectionIterator& operator++() {
            ++nextValueArrayIndex;

			bool foundIntersection = false;
            while (! isEnd && ! foundIntersection) {

				// Search for the next node if we've exhausted this one
                while ((! isEnd) &&  (nextValueArrayIndex >= node->valueArray.length())) {
					// If we entered this loop, then the iterator has exhausted the elements at 
					// node (possibly because it just switched to a child node with no members).
					// This loop continues until it finds a node with members or reaches
					// the end of the whole intersection search.

					// If the right child overlaps the box, push it onto the stack for
					// processing.
					if ((node->child[1] != NULL) &&
						(box.high()[node->splitAxis] > node->splitLocation)) {
						stack.push(node->child[1]);
					}
                
					// If the left child overlaps the box, push it onto the stack for
					// processing.
					if ((node->child[0] != NULL) &&
						(box.low()[node->splitAxis] < node->splitLocation)) {
						stack.push(node->child[0]);
					}

					if (stack.length() > 0) {
						// Go on to the next node (which may be either one of the ones we 
						// just pushed, or one from farther back the tree).
						node = stack.pop();
						nextValueArrayIndex = 0;
					} else {
						// That was the last node; we're done iterating
						isEnd = true;
					}
				}

				// Search for the next intersection at this node until we run out of children
				while (! isEnd && ! foundIntersection && (nextValueArrayIndex < node->valueArray.length())) {
					if (box.intersects(node->valueArray[nextValueArrayIndex].bounds)) {
						foundIntersection = true;
					} else {
						++nextValueArrayIndex;
						// If we exhaust this node, we'll loop around the master loop 
						// to find a new node.
					}
				}
            }

            return *this;
        }

        /**
         Post increment (much slower than preincrement!).
         */
        BoxIntersectionIterator operator++(int) {
            BoxIntersectionIterator old = *this;
            ++this;
            return old;
        }

        /** Overloaded dereference operator so the iterator can masquerade as a pointer
            to a member */
        const T& operator*() const {
            alwaysAssertM(! isEnd, "Can't dereference the end element of an iterator");
            return node->valueArray[nextValueArrayIndex].value;
        }

        /** Overloaded dereference operator so the iterator can masquerade as a pointer
            to a member */
        T const * operator->() const {
            alwaysAssertM(! isEnd, "Can't dereference the end element of an iterator");
            return &(stack.last()->valueArray[nextValueArrayIndex].value);
        }

        /** Overloaded cast operator so the iterator can masquerade as a pointer
            to a member */
        operator T*() const {
            alwaysAssertM(! isEnd, "Can't dereference the end element of an iterator");
            return &(stack.last()->valueArray[nextValueArrayIndex].value);
        }
    };


    /**
     Iterates through the members that intersect the box
     */
    BoxIntersectionIterator beginBoxIntersection(const AABox& box) const {
        return BoxIntersectionIterator(box, root);
    }

    BoxIntersectionIterator endBoxIntersection() const {
        // The "end" iterator instance
        return BoxIntersectionIterator();
    }

    /**
     Appends all members whose bounds intersect the box.
     See also PointAABSPTree::beginBoxIntersection.
     */
    void getIntersectingMembers(const AABox& box, Array<T>& members) const {
        if (root == NULL) {
            return;
        }
        root->getIntersectingMembers(box, Sphere(Vector3::zero(), 0), members, false);
    }


    /**
      @param members The results are appended to this array.
     */
    void getIntersectingMembers(const Sphere& sphere, Array<T>& members) const {
        if (root == NULL) {
            return;
        }

        AABox box;
        sphere.getBounds(box);
        root->getIntersectingMembers(box, sphere, members, true);

    }


    /**
      Stores the locations of the splitting planes (the structure but not the content)
      so that the tree can be quickly rebuilt from a previous configuration without 
      calling balance.
     */
    void serializeStructure(BinaryOutput& bo) const {
        Node::serializeStructure(root, bo);
    }

    /** Clears the member table */
    void deserializeStructure(BinaryInput& bi) {
        clear();
        root = Node::deserializeStructure(bi);
    }

    /**
     Returns an array of all members of the set.  See also PointAABSPTree::begin.
     */
    void getMembers(Array<T>& members) const {
        memberTable.getKeys(members);
    }


    /**
     C++ STL style iterator variable.  See begin().
     Overloads the -> (dereference) operator, so this acts like a pointer
     to the current member.
    */
    class Iterator {
    private:
        friend class PointAABSPTree<T>;

        // Note: this is a Table iterator, we are currently defining
        // Set iterator
        typename Table<T, Node*>::Iterator it;

        Iterator(const typename Table<T, Node*>::Iterator& it) : it(it) {}

    public:
        inline bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        bool operator==(const Iterator& other) const {
            return it == other.it;
        }

        /**
         Pre increment.
         */
        Iterator& operator++() {
            ++it;
            return *this;
        }

        /**
         Post increment (slower than preincrement).
         */
        Iterator operator++(int) {
            Iterator old = *this;
            ++(*this);
            return old;
        }

        const T& operator*() const {
            return it->key;
        }

        T* operator->() const {
            return &(it->key);
        }

        operator T*() const {
            return &(it->key);
        }
    };


    /**
     C++ STL style iterator method.  Returns the first member.  
     Use preincrement (++entry) to get to the next element (iteration
     order is arbitrary).  
     Do not modify the set while iterating.
     */
    Iterator begin() const {
        return Iterator(memberTable.begin());
    }


    /**
     C++ STL style iterator method.  Returns one after the last iterator
     element.
     */
    Iterator end() const {
        return Iterator(memberTable.end());
    }
};

}

#endif
