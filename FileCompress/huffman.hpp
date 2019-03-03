#pragma once
#include <queue>
#include <vector>

//模板不支持分离编译  .hpp声明定义在一起
template <class W>
struct HTNode{
	HTNode(const W& weight)
		: _pLeft(nullptr)
		, _pRight(nullptr)
		, _pParent(nullptr)
		, _weight(weight)
	{}

	HTNode<W>* _pLeft;
	HTNode<W>* _pRight;
	HTNode<W>* _pParent;
	W _weight;
};

//仿函数----不是函数,是类
//重载了()运算符,可以像调用函数那样子调用
template<class W>
struct Compare{
	bool operator()(HTNode<W>* pLeft, HTNode<W>*  pRight){
		if (pLeft->_weight > pRight->_weight)
			return true;
		return false;
	}
};

template<class W>
class HuffmanTree{
	typedef HTNode<W> Node;
	typedef Node* PNode;

public:
	HuffmanTree()
		:_pRoot(nullptr)
	{}
	~HuffmanTree(){
		Destroy(_pRoot);
	}

	void CreatHuffmanTree(const std::vector<W>& v,const W& invalid){
		if (v.empty())
			return;

		//小堆---创建二叉树的森林
		priority_queue<PNode, std::vector<PNode>,Compare<W>> hp;
		for (size_t i = 0; i < v.size(); i++){
			if (v[i]!=invalid)
				hp.push(new Node(v[i]));
		}

		while (hp.size()>1){
			PNode pLeft = hp.top();
			hp.pop();

			PNode pRight = hp.top();
			hp.pop();

			//以权值和创建新的结点
			PNode pParent = new Node(pLeft->_weight + pRight->_weight);
			
			pParent->_pLeft = pLeft;
			pLeft->_pParent = pParent;

			pParent->_pRight = pRight;
			pRight->_pParent = pParent;

			hp.push(pParent);
		}
		_pRoot = hp.top();
	}

	PNode GetRoot(){
		return _pRoot;
	}
private:
	void Destroy(PNode& pRoot){
		if (pRoot){
			Destroy(pRoot->_pLeft);
			Destroy(pRoot->_pRight);
			delete(pRoot);
			pRoot = nullptr;
		}
	}
private:
	PNode _pRoot;
};