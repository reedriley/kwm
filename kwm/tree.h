#ifndef TREE_H
#define TREE_H

#include "types.h"
#include "../axlib/display.h"

tree_node *CreateTreeFromWindowIDList(ax_display *Display, std::vector<uint32_t> *Windows);
void FillDeserializedTree(tree_node *RootNode, ax_display *Display, std::vector<uint32_t> *WindowsPtr);
void RotateBSPTree(int Deg);
void EqualizeBSPTree(std::string Op);
tree_node * FindFirstMinDepthLeafNode(tree_node *Root);
tree_node *GetNearestLeafNodeNeighbour(tree_node *Node);
tree_node *GetTreeNodeForPoint(tree_node *Node, CGPoint *Point);
tree_node *GetTreeNodeFromWindowID(tree_node *Node, uint32_t WindowID);
tree_node *GetTreeNodeFromWindowIDOrLinkNode(tree_node *Node, uint32_t WindowID);
link_node *GetLinkNodeFromWindowID(tree_node *Root, uint32_t WindowID);
link_node *GetLinkNodeFromTree(tree_node *Root, uint32_t WindowID);
tree_node *GetTreeNodeFromLink(tree_node *Root, link_node *Link);
tree_node *GetNearestTreeNodeToTheLeft(tree_node *Node);
tree_node *GetNearestTreeNodeToTheRight(tree_node *Node);
void GetFirstLeafNode(tree_node *Node, void **Result);
void GetLastLeafNode(tree_node *Node, void **Result);
void FocusFirstLeafNode(ax_display *Display);
void FocusLastLeafNode(ax_display *Display);
tree_node *GetFirstPseudoLeafNode(tree_node *Node);
void ApplyLinkNodeContainer(link_node *Link);
void ApplyTreeNodeContainer(tree_node *Node);
void DestroyNodeTree(tree_node *Node);

#endif
