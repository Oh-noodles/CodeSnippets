#include <algorithm>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>

class Node {
  public:
    float x1, y1;
    float x2, y2;
    Node *parent = NULL;
    Node *left = NULL;
    Node *right = NULL;
    bool isLeaf = true;

    Node(float x1 = 0, float y1 = 0, float x2 = 0, float y2 = 0, bool isLeaf = true) {
      this->x1 = x1;
      this->y1 = y1;
      this->x2 = x2;
      this->y2 = y2;
      this->isLeaf = isLeaf;
    }
};

float getSA(Node *node) {
  if (!node) return 0;
  return 2.0f * (node->x2 - node->x1 + node->y2 - node->y1);
}

float getUnionSA(std::vector<Node *> nodes) {
  Node *node = nodes[0];
  float x1 = 0; // node->x1;
  float y1 = 0; // node->y1;
  float x2 = 0; // node->x2;
  float y2 = 0; // node->y2;
  bool first = true;
  
  for (int i = 0; i < nodes.size(); i++) {
    node = nodes[i];
    if (node) {
      if (first) {
        first = false;
        x1 = node->x1;
        y1 = node->y1;
        x2 = node->x2;
        y2 = node->y2;
      }
      x1 = std::min(x1, node->x1);
      y1 = std::min(y1, node->y1);
      x2 = std::max(x2, node->x2);
      y2 = std::max(y2, node->y2);
    }
  }
  if (first) assert("getUnionSA: all nodes are invalid");
  return 2.0f * (x2 - x1 + y2 - y1);
}

Node* findBestSibling(Node *node, Node *leaf) {
  while (node && !node->isLeaf) {
    float cost = getUnionSA({node, leaf});
    float delta = cost - getSA(node);
    float costLeft = node->left ? getUnionSA({node->left, leaf}) : cost + 1.0f;
    float costRight = node->right ? getUnionSA({node->right, leaf}) : cost + 1.0f;
    if (cost < costLeft && cost < costRight)
      break;
    if (costLeft < costRight)
      node = node->left;
    else
      node = node->right;
  }
  return node;
}

void refit(Node *node) {
  Node *left = node->left;
  Node *right = node->right;
  if (left && right) {
    node->x1 = std::min(left->x1, right->x1);
    node->y1 = std::min(left->y1, right->y1);
    node->x2 = std::max(left->x2, right->x2);
    node->y2 = std::max(left->y2, right->y2);
  } else {
    Node *nd = left ? left : right;
    node->x1 = nd->x1;
    node->y1 = nd->y1;
    node->x2 = nd->x2;
    node->y2 = nd->y2;
  }
}

void rotate(Node *node) {
  Node *left = node->left;
  Node *right = node->right;
  if (!(left && right)) return;

  Node *ll = left->left;
  Node *lr = left->right;
  Node *rl = right->left;
  Node *rr = right->right;

  // ==== compute delta SA for 4 types of rotations ====
  float SA_B = getSA(left);
  float SA_C = getSA(right);

  float SA_FGE = getUnionSA({rl, rr, lr});
  float SA_DFG = getUnionSA({ll, rl, rr});
  float SA_DEG = getUnionSA({ll, lr, rr});
  float SA_FDE = getUnionSA({rl, ll, lr});

  float delta1 = SA_FGE - SA_B;
  float delta2 = SA_DFG - SA_B;
  float delta3 = SA_DEG - SA_C;
  float delta4 = SA_FDE - SA_C;

  // ==== choose the one with max drop ====
  std::array<float, 4> deltas = { delta1, delta2, delta3, delta4 };
  float minDelta = 0;
  int pickIndex = -1;
  for (int i = 0; i < 4; i++) {
    if (deltas[i] < minDelta) {
      minDelta = deltas[i];
      pickIndex = i;
    }
  }

  switch (pickIndex) {
    case -1:
      // do not rotate
      break;
    case 0:
      // rotate type 1
      node->right = ll;
      ll && (ll->parent = node);
      left->left = right;
      right->parent = left;
      refit(left);
      break;
    case 1:
      // rotate type 2
      node->right = lr;
      lr && (lr->parent = node);
      left->right = right;
      right->parent = left;
      refit(left);
      break;
    case 2:
      // rotate type 3
      node->left = rl;
      rl && (rl->parent = node);
      right->left = left;
      left->parent = right;
      refit(right);
      break;
    case 3:
      // rotate type 4
      node->left = rr;
      rr && (rr->parent = node);
      right->right = left;
      left->parent = right;
      refit(right);
      break;
    default:
      break;
  }
}

class Tree {
  public:
    Node *dummyRoot;

    Tree() {
      dummyRoot = new Node();
    }

    void insertLeaf(Node *newLeaf) {
      if (dummyRoot->left == NULL) {
        dummyRoot->left = newLeaf;
        newLeaf->parent = dummyRoot;
        return;
      }

      // ==== stage 1: find the best sibling ====
      Node *sibling = findBestSibling(dummyRoot->left, newLeaf);
      // ==== stage 2: create new parent add insert ====
      Node *newParent = new Node(
          std::min(sibling->x1, newLeaf->x1),
          std::min(sibling->y1, newLeaf->y1),
          std::max(sibling->x2, newLeaf->x2),
          std::max(sibling->y2, newLeaf->y2),
          false
        );
      Node *oldParent = sibling->parent;
      if (oldParent->left == sibling)
        oldParent->left = newParent;
      else
        oldParent->right = newParent;
      newParent->parent = oldParent;
      newParent->left = sibling;
      newParent->right = newLeaf;
      sibling->parent = newParent;
      newLeaf->parent = newParent;
      // ==== stage 3: trackback ro root, refit and rotate ====
      Node *parent = oldParent;
      while (parent) {
        refit(parent);
        rotate(parent);
        parent = parent->parent;
      }
    }

    void removeLeaf(Node *rmLeaf) {
      // delete
      Node *parent = rmLeaf->parent;
      if (parent->left == rmLeaf)
        parent->left = NULL;
      else
        parent->right = NULL;

      // trackback to root, refit and rotate
      while (parent) {
        refit(parent);
        rotate(parent);
        parent = parent->parent;
      }
    }

    void updateLeaf(Node *updLeaf, float x1, float y1, float x2, float y2) {
      // remove
      removeLeaf(updLeaf);
      // insert with updated value
      insertLeaf(new Node(x1, y1, x2, y2, true));
    }
};

void printTree(Node *node, std::string prefix, bool isLeft) {
  if (node == NULL) return;

  std::vector<std::string> leftSymbols = { "├── ", "│   " };
  std::vector<std::string> rightSymbols = { "└── ", "    " };

  std::vector<std::string> symbols = isLeft ? leftSymbols : rightSymbols;

  std::cout << prefix << symbols[0];
  std::cout << (node->isLeaf ? "\033[1;32m" : "\033[1;31m");
  std::cout << node->x1 << "," << node->y1 << "," << node->x2 << "," << node->y2 << std::endl;
  std::cout << "\033[0m";
  printTree(node->left, prefix + symbols[1], true);
  printTree(node->right, prefix + symbols[1], false);
}

std::string getSvgPath(Node *node, std::string color) {
  std::stringstream s;
  float xMin = node->x1;
  float yMin = node->y1;
  float xMax = node->x2;
  float yMax = node->y2;

  if (!node->isLeaf) {

    const float padding = 0.5;
    xMin -= padding;
    yMin -= padding;
    xMax += padding;
    yMax += padding;
  }
  
  s << "  <path stroke=\""<< color <<"\" fill-opacity=\"0\" stroke-width=\"0.2\""; 
  s <<" d= \"";
  s << "M " << xMin << " " << yMin << " L " << xMax << " " << yMin << " L " << xMax << " " << yMax << " L " << xMin << " " << yMax << " L " << xMin << " " << yMin;
  s <<  "\"/>\n";
  return s.str();
}

void renderNode(Node *node, std::ofstream &file) {
  if (!node) return;

  std::string color = node->isLeaf ? "green" : "red";
  file << getSvgPath(node, color);
  renderNode(node->left, file);
  renderNode(node->right, file);
}

void renderTree(Tree *tree, std::string filename) {
  std::ofstream file;
  file.open(filename);
  file << "<svg width=\"1000\" height=\"1000\" viewBox=\"-20 -20 140 140\" xmlns=\"http://www.w3.org/2000/svg\">\n";
  file.close();
  file.open(filename, std::ios_base::app);
  renderNode(tree->dummyRoot->left, file);
  file << "</svg>\n";
  file.close();
}

int randint(int mn = 0, int mx = 100) {
  std::srand(std::time(nullptr) * std::rand());
  int val = abs(std::rand());
  val = mn + (val % (mx - mn));
  return val;
} 

int main() {
  Tree *tree = NULL;

  // ==== random test ====
  tree = new Tree();
  for (int i = 0; i < 6; i++) {
    int x = randint(1, 100);
    int y = randint(1, 100);
    int w = randint(1, 6);
    int h = randint(1, 6);
    Node *node = new Node(x, y, x + w, y + h);
    tree->insertLeaf(node);
  }
  std::cout << "random test result: " << std::endl;
  printTree(tree->dummyRoot->left, "", false);
  renderTree(tree, "output1.svg");

  // ==== sorted input test ====
  tree = new Tree();
  for (int i = 0; i < 5; i++) {
    float x = i * 10;
    float y = 0;
    Node *node = new Node(x, y, x + 6, y + 6);
    tree->insertLeaf(node);
  }
  std::cout << "sorted input test result: " << std::endl;
  printTree(tree->dummyRoot->left, "", false);
  renderTree(tree, "output2.svg");
}
