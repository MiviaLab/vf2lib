/*-------------------------------------------------------------------
 * argraph.cc
 * Implementation of the ARGraph_impl class and of related classes 
 *
 * Author: P. Foggia
 ------------------------------------------------------------------*/


#include <string.h>
#include <stddef.h>

#include "argraph.h"
#include "error.h"

/*---------------------------------------------------------------
 * MACROS & inline
 --------------------------------------------------------------*/
#define VCOPY(dest, src, type, nelem) memcpy((dest),(src),(nelem)*sizeof(type))

template <class T>
inline void grow(T* &ptr, int old_size, int new_size)
  { T* nptr=new T[new_size];
    ptrcheck(nptr);
    VCOPY(nptr, ptr, T, old_size);
    delete[] ptr;
    ptr=nptr;
  };

template <class T>
inline void clear(T* ptr, int n)
  { int i;
    for(i=0; i<n; i++)
      ptr[i]=(T)0;
  }

inline void clear(byte *ptr, int n)
  { memset(ptr, 0, n);
  }


/*---------------------------------------------------------------
 * Static prototypes
 --------------------------------------------------------------*/
static void ptrcheck(void *);

/*----------------------------------------------------------------
 * methods of the class ARGraph_impl
 ---------------------------------------------------------------*/

/*---------------------------------------------------------------
 * Constructor
 --------------------------------------------------------------*/
ARGraph_impl::ARGraph_impl(ARGLoader *loader)
  { node_destroyer=NULL;
    edge_destroyer=NULL;
    node_comparator=NULL;
    edge_comparator=NULL;
    n = loader->NodeCount();
    attr = new void*[n];
    ptrcheck(attr);
    int i, j;
    for(i=0; i<n; i++)
      attr[i]=loader->GetNodeAttr(i);
    in_count=new count_type[n];
    ptrcheck(in_count);
    clear(in_count, n);
    out_count=new count_type[n];
    ptrcheck(out_count);
    clear(out_count, n);
    in=new node_id*[n];
    ptrcheck(in);
    clear(in, n);
    out=new node_id*[n];
    ptrcheck(out);
    clear(out, n);
    in_attr=new void**[n];
    ptrcheck(in_attr);
    clear(in_attr, n);
    out_attr=new void**[n];
    ptrcheck(out_attr);
    clear(out_attr, n);
    for(i=0; i<n; i++)
      { int k=out_count[i]=loader->OutEdgeCount(i);
        out[i]=new node_id[k];
        ptrcheck(out[i]);
        out_attr[i]=new void *[k];
        ptrcheck(out_attr[i]);
        for(j=0; j<k; j++)
          { node_id n2=out[i][j]=loader->GetOutEdge(i, j, &out_attr[i][j]);
            in_count[n2]++;
          }
      }
    
    for(i=0; i<n; i++)
      { int k=in_count[i];
        in[i]=new node_id[k];
        ptrcheck(in[i]);
        in_attr[i]=new void *[k];
        ptrcheck(in_attr[i]);
        int l=0;
        for(j=0; j<n; j++)
          { if (HasEdge(j,i))
              { in[i][l]=j;
                in_attr[i][l]=GetEdgeAttr(j, i);
                l++;
              }
          }
        assert(l==k);
      }
  }

/*-------------------------------------------------
 * ARGraph_impl::~ARGraph_impl()
 * Destructor.
 * Frees the memory allocated for the graph
 ------------------------------------------------*/
ARGraph_impl::~ARGraph_impl()
  { int i,j;
    for(i=0; i<n; i++)
      { 
        for(j=0; j<out_count[i]; j++)
            DestroyEdge(out_attr[i][j]);
        delete[] in[i];
        delete[] in_attr[i];
        delete[] out[i];
        delete[] out_attr[i];
      }

    for(i=0; i<n; i++)
      DestroyNode(attr[i]);

    delete[] attr;
    delete[] in_attr;
    delete[] out_attr;
    delete[] in_count;
    delete[] in;
    delete[] out_count;
    delete[] out;
    
    delete node_destroyer;
    delete edge_destroyer;
    delete node_comparator;
    delete edge_comparator;
  }

/*-------------------------------------------------------------------
 * Set the object to invoke to destroy node attributes
 * Note:
 *   The object is owned by the graph; i.e. it is 
 *   deleted when the graph is deallocated.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetNodeDestroyer(AttrDestroyer *dest)
  { delete node_destroyer;
    node_destroyer=dest;
  }

/*-------------------------------------------------------------------
 * Set the function to invoke to destroy node attributes.
 * Note:
 *   This function is provided for compatibility with older
 *   versions of the library. It creates an object of the class 
 *   FunctionAttrDestroyer.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetNodeDestroy(node_destroy_fn fn)
  { SetNodeDestroyer(new FunctionAttrDestroyer(fn));
  }

/*-------------------------------------------------------------------
 * Set the object to invoke to destroy edge attributes
 * Note:
 *   The object is owned by the graph; i.e. it is 
 *   deleted when the graph is deallocated.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetEdgeDestroyer(AttrDestroyer *dest)
  { delete edge_destroyer;
    edge_destroyer=dest;
  }

/*-------------------------------------------------------------------
 * Set the function to invoke to destroy edge attributes
 * Note:
 *   This function is provided for compatibility with older
 *   versions of the library. It creates an object of the class 
 *   FunctionAttrDestroyer.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetEdgeDestroy(edge_destroy_fn fn)
  { SetNodeDestroyer(new FunctionAttrDestroyer(fn));
  }

/*-------------------------------------------------------------------
 * Set the object to invoke to compare node attributes
 * Note:
 *   The object is owned by the graph; i.e. it is 
 *   deleted when the graph is deallocated.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetNodeComparator(AttrComparator *comp)
  { delete node_comparator;
    node_comparator=comp;
  }

/*-------------------------------------------------------------------
 * Set the function to invoke to test for node compatibility
 * Note:
 *   This function is provided for compatibility with older
 *   versions of the library. It creates an object of the class 
 *   FunctionAttrComparator.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetNodeCompat(node_compat_fn fn)
  { SetNodeComparator(new FunctionAttrComparator(fn));
  }

/*-------------------------------------------------------------------
 * Set the object to invoke to compare edge attributes
 * Note:
 *   The object is owned by the graph; i.e. it is 
 *   deleted when the graph is deallocated.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetEdgeComparator(AttrComparator *comp)
  { delete edge_comparator;
    edge_comparator=comp;
  }

/*-------------------------------------------------------------------
 * Set the function to invoke to test for edge compatibility
 * Note:
 *   This function is provided for compatibility with older
 *   versions of the library. It creates an object of the class 
 *   FunctionAttrComparator.
 ------------------------------------------------------------------*/
void ARGraph_impl::SetEdgeCompat(edge_compat_fn fn)
  { SetEdgeComparator(new FunctionAttrComparator(fn));
  }

/*-------------------------------------------------------------------
 * Change the attribute of a node
 -------------------------------------------------------------------*/
void ARGraph_impl::SetNodeAttr(node_id i, void *new_attr, bool destroyOld)
  { assert(i<n);
    if (destroyOld)
      DestroyNode(attr[i]);
    attr[i]=new_attr;
  }

/*-------------------------------------------------------------------
 * Checks the existence of an edge, and returns its attribute
 * using the parameter pattr.
 * Note: uses binary search.
 * Implem. note: Uses the out/out_attr vectors; this fact is 
 *               exploited in the constructor to generate the 
 *               in_attr vector
 ------------------------------------------------------------------*/
bool ARGraph_impl::HasEdge(node_id n1, node_id n2, void **pattr)
  { register int a, b, c;
    node_id *id=out[n1];

    assert(n1<n);
    assert(n2<n);

    a=0;
    b=out_count[n1];
    while (a<b)
      { c=(unsigned)(a+b)>>1;
        if (id[c]<n2)
          a=c+1;
        else if (id[c]>n2)
          b=c;
        else
          { if (pattr)
		      *pattr=out_attr[n1][c];
			return true;
		  }
      }
    return false;
  }

/*-------------------------------------------------------------------
 * Change the attribute of an edge. It is an error if the edge
 * does not exist.
 * Note: uses binary search.
 ------------------------------------------------------------------*/
void  ARGraph_impl::SetEdgeAttr(node_id n1, node_id n2, void *new_attr,
                                bool destroyOld)
  { register int a, b, c;
    node_id *id;

    assert(n1<n);
    assert(n2<n);

    //
    // Replace the attribute in the out_attr array
    //
    id=out[n1];
    a=0;
    b=out_count[n1];
    while (a<b)
      { c=(unsigned)(a+b)>>1;
        if (id[c]<n2)
          a=c+1;
        else if (id[c]>n2)
          b=c;
        else
          { if (destroyOld)
              DestroyEdge(out_attr[n1][c]);
            out_attr[n1][c]=new_attr;
	    break;
          }
      }
    if (a>=b)
      error("ARGraph_impl::SetEdgeAttr: non existent edge");
   
    //
    // Replace the attribute in the in_attr array
    //
    id = in[n2];
    a=0;
    b=in_count[n2];
    while (a<b)
      { c=(unsigned)(a+b)>>1;
        if (id[c]<n1)
          a=c+1;
        else if (id[c]>n1)
          b=c;
        else
          { // The old attr here is intentionally
	    // not destroyed with DestroyEdge, since destruction
	    // has been performed previously through out_attr
            in_attr[n2][c]=new_attr;
	    break;
          }
      }
    if (a>=b)
      error("ARGraph_impl::SetEdgeAttr: inconsistent graph state");
  }


/*-------------------------------------------------------------------
 * void ARGraph_impl::VisitInEdges(node, vis, param)
 * Applies the visitor to all the 'in' edges of 'node'
 ------------------------------------------------------------------*/
void ARGraph_impl::VisitInEdges(node_id node, edge_visitor vis, 
                                param_type param)
  { 
    assert(node<n);
    int i;
    for(i=0; i<in_count[node]; i++)
      vis(this, in[node][i], node, in_attr[node][i], param);
  }

/*-------------------------------------------------------------------
 * void ARGraph_impl::VisitOutEdges(node, vis, param)
 * Applies the visitor to all the 'out' edges of 'node'
 ------------------------------------------------------------------*/
void ARGraph_impl::VisitOutEdges(node_id node, edge_visitor vis, 
                                 param_type param)
  {
    assert(node<n); 
    int i;
    for(i=0; i<out_count[node]; i++)
      vis(this, node, out[node][i], out_attr[node][i], param);
  }

/*-------------------------------------------------------------------
 * void ARGraph_impl::VisitEdges(node, vis, param)
 * Applies the visitor to all the edges of 'node'
 ------------------------------------------------------------------*/
void ARGraph_impl::VisitEdges(node_id node, edge_visitor vis, param_type param)
  { VisitInEdges(node, vis, param);
    VisitOutEdges(node, vis, param);
  }


/*----------------------------------------------------------------------
 * Destroy the attribute of a node
 ---------------------------------------------------------------------*/
void ARGraph_impl::DestroyNode(void *attr)
  { if (node_destroyer!=NULL)
      node_destroyer->destroy(attr);
  }


/*----------------------------------------------------------------------
 * Destroy the attribute of an edge
 ---------------------------------------------------------------------*/
void ARGraph_impl::DestroyEdge(void *attr)
  { if (edge_destroyer!=NULL)
      edge_destroyer->destroy(attr);
  }


/***********************************************************
 * Utility classes
 **********************************************************/
FunctionAttrDestroyer::FunctionAttrDestroyer(void (*fn)(void *))
  { func = fn;
  }

void FunctionAttrDestroyer::destroy(void *attr)
  { if (func!=NULL)
      func(attr);
  }

FunctionAttrComparator::FunctionAttrComparator(bool (*fn)(void *, void *))
  { func = fn;
  }

bool FunctionAttrComparator::compatible(void *attr1, void *attr2)
  { if (func!=NULL)
      return func(attr1, attr2);
    else
      return true;
  }



/*-------------------------------------------------------------
 * Static functions
 ------------------------------------------------------------*/
static void ptrcheck(void *p)
  { if (p==NULL)
      error("Out of memory");
  }
