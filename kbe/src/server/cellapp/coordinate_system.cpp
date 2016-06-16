/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2016 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "coordinate_node.h"
#include "coordinate_system.h"
#include "profile.h"

#ifndef CODE_INLINE
#include "coordinate_system.inl"
#endif

namespace KBEngine{	

bool CoordinateSystem::hasY = false;

//-------------------------------------------------------------------------------------
CoordinateSystem::CoordinateSystem():
size_(0),
first_x_coordinateNode_(NULL),
first_y_coordinateNode_(NULL),
first_z_coordinateNode_(NULL),
dels_(),
dels_count_(0),
updating_(0)
{
}

//-------------------------------------------------------------------------------------
CoordinateSystem::~CoordinateSystem()
{
	dels_.clear();
	dels_count_ = 0;

	if(first_x_coordinateNode_)
	{
		CoordinateNode* pNode = first_x_coordinateNode_;
		while(pNode != NULL)
		{
			CoordinateNode* pNextNode = pNode->pNextX();
			pNode->pCoordinateSystem(NULL);
			pNode->pPrevX(NULL);
			pNode->pNextX(NULL);
			pNode->pPrevY(NULL);
			pNode->pNextY(NULL);
			pNode->pPrevZ(NULL);
			pNode->pNextZ(NULL);
			
			delete pNode;

			pNode = pNextNode;
		}
		
		// 上面已经销毁过了
		first_x_coordinateNode_ = NULL;
		first_y_coordinateNode_ = NULL;
		first_z_coordinateNode_ = NULL;
	}
}

//-------------------------------------------------------------------------------------
bool CoordinateSystem::insert(CoordinateNode* pNode)
{
	// 如果链表是空的, 初始第一个和最后一个xz节点为该节点
	if(isEmpty())
	{
		first_x_coordinateNode_ = pNode;

		if(CoordinateSystem::hasY)
			first_y_coordinateNode_ = pNode;

		first_z_coordinateNode_ = pNode;

		pNode->pPrevX(NULL);
		pNode->pNextX(NULL);
		pNode->pPrevY(NULL);
		pNode->pNextY(NULL);
		pNode->pPrevZ(NULL);
		pNode->pNextZ(NULL);
		pNode->x(pNode->xx());
		pNode->y(pNode->yy());
		pNode->z(pNode->zz());
		pNode->pCoordinateSystem(this);

		size_ = 1;
		
		// 只有一个节点不需要更新
		// update(pNode);
		pNode->resetOld();
		return true;
	}

	pNode->old_xx(-FLT_MAX);
	pNode->old_yy(-FLT_MAX);
	pNode->old_zz(-FLT_MAX);

	pNode->x(first_x_coordinateNode_->x());
	first_x_coordinateNode_->pPrevX(pNode);
	pNode->pNextX(first_x_coordinateNode_);
	first_x_coordinateNode_ = pNode;

	if(CoordinateSystem::hasY)
	{
		pNode->y(first_y_coordinateNode_->y());
		first_y_coordinateNode_->pPrevY(pNode);
		pNode->pNextY(first_y_coordinateNode_);
		first_y_coordinateNode_ = pNode;
	}
	
	pNode->z(first_z_coordinateNode_->z());
	first_z_coordinateNode_->pPrevZ(pNode);
	pNode->pNextZ(first_z_coordinateNode_);
	first_z_coordinateNode_ = pNode;

	pNode->pCoordinateSystem(this);
	++size_;

	update(pNode);
	return true;
}

//-------------------------------------------------------------------------------------
bool CoordinateSystem::remove(CoordinateNode* pNode)
{
	// 如果已经处于删除中的状态，就表示当前处于删除流程中，不需要再次进入流程
	if ((pNode->flags() & (COORDINATE_NODE_FLAG_REMOVED | COORDINATE_NODE_FLAG_REMOVEING)))
		return false;

	pNode->flags(pNode->flags() | COORDINATE_NODE_FLAG_REMOVEING);
	pNode->onRemove();
	
	if((pNode->flags() & COORDINATE_NODE_FLAG_PENDING) > 0)
	{
		pNode->flags(pNode->flags() | COORDINATE_NODE_FLAG_REMOVED);
		std::list<CoordinateNode*>::iterator iter = std::find(dels_.begin(), dels_.end(), pNode);
		if(iter == dels_.end())
		{
			dels_.push_back(pNode);
			++dels_count_;
		}
	}
	else
	{
		update(pNode);
		pNode->flags(pNode->flags() | COORDINATE_NODE_FLAG_REMOVED);
		removeReal(pNode);
	}

	return true;
}

//-------------------------------------------------------------------------------------
void CoordinateSystem::removeDelNodes()
{
	if(dels_count_ == 0)
		return;

	std::list<CoordinateNode*>::iterator iter = dels_.begin();
	for(; iter != dels_.end(); ++iter)
	{
		removeReal((*iter));
	}

	dels_.clear();
	dels_count_ = 0;
}

//-------------------------------------------------------------------------------------
bool CoordinateSystem::removeReal(CoordinateNode* pNode)
{
	if(pNode->pCoordinateSystem() == NULL)
	{
		return true;
	}

	// 如果是第一个节点
	if(first_x_coordinateNode_ == pNode)
	{
		first_x_coordinateNode_ = first_x_coordinateNode_->pNextX();

		if(first_x_coordinateNode_)
		{
			first_x_coordinateNode_->pPrevX(NULL);
		}
	}
	else
	{
		pNode->pPrevX()->pNextX(pNode->pNextX());

		if(pNode->pNextX())
			pNode->pNextX()->pPrevX(pNode->pPrevX());
	}

	if(CoordinateSystem::hasY)
	{
		// 如果是第一个节点
		if(first_y_coordinateNode_ == pNode)
		{
			first_y_coordinateNode_ = first_y_coordinateNode_->pNextY();

			if(first_y_coordinateNode_)
			{
				first_y_coordinateNode_->pPrevY(NULL);
			}
		}
		else
		{
			pNode->pPrevY()->pNextY(pNode->pNextY());

			if(pNode->pNextY())
				pNode->pNextY()->pPrevY(pNode->pPrevY());
		}
	}

	// 如果是第一个节点
	if(first_z_coordinateNode_ == pNode)
	{
		first_z_coordinateNode_ = first_z_coordinateNode_->pNextZ();

		if(first_z_coordinateNode_)
		{
			first_z_coordinateNode_->pPrevZ(NULL);
		}
	}
	else
	{
		pNode->pPrevZ()->pNextZ(pNode->pNextZ());

		if(pNode->pNextZ())
			pNode->pNextZ()->pPrevZ(pNode->pPrevZ());
	}

	pNode->pPrevX(NULL);
	pNode->pNextX(NULL);
	pNode->pPrevY(NULL);
	pNode->pNextY(NULL);
	pNode->pPrevZ(NULL);
	pNode->pNextZ(NULL);
	pNode->pCoordinateSystem(NULL);
	releases_.push_back(pNode);

	--size_;
	return true;
}

//-------------------------------------------------------------------------------------
void CoordinateSystem::moveNodeX(CoordinateNode* pNode, float px, CoordinateNode* pCurrNode)
{
	if(pCurrNode != NULL)
	{
		if(pCurrNode->x() > px)
		{
			CoordinateNode* pPreNode = pCurrNode->pPrevX();
			pCurrNode->pPrevX(pNode);
			if(pPreNode)
			{
				pPreNode->pNextX(pNode);
				if(pNode == first_x_coordinateNode_ && pNode->pNextX())
					first_x_coordinateNode_ = pNode->pNextX();
			}
			else
			{
				first_x_coordinateNode_ = pNode;
			}

			if(pNode->pPrevX())
				pNode->pPrevX()->pNextX(pNode->pNextX());

			if(pNode->pNextX())
				pNode->pNextX()->pPrevX(pNode->pPrevX());
			
			pNode->pPrevX(pPreNode);
			pNode->pNextX(pCurrNode);
		}
		else
		{
			CoordinateNode* pNextNode = pCurrNode->pNextX();
			if(pNextNode != pNode)
			{
				pCurrNode->pNextX(pNode);
				if(pNextNode)
					pNextNode->pPrevX(pNode);

				if(pNode->pPrevX())
					pNode->pPrevX()->pNextX(pNode->pNextX());

				if(pNode->pNextX())
				{
					pNode->pNextX()->pPrevX(pNode->pPrevX());
				
					if(pNode == first_x_coordinateNode_)
						first_x_coordinateNode_ = pNode->pNextX();
				}

				pNode->pPrevX(pCurrNode);
				pNode->pNextX(pNextNode);
			}
		}
	}
}

//-------------------------------------------------------------------------------------
void CoordinateSystem::moveNodeY(CoordinateNode* pNode, float py, CoordinateNode* pCurrNode)
{
	if(pCurrNode != NULL)
	{
		if(pCurrNode->y() > py)
		{
			CoordinateNode* pPreNode = pCurrNode->pPrevY();
			pCurrNode->pPrevY(pNode);
			if(pPreNode)
			{
				pPreNode->pNextY(pNode);
				if(pNode == first_y_coordinateNode_ && pNode->pNextY())
					first_y_coordinateNode_ = pNode->pNextY();
			}
			else
			{
				first_y_coordinateNode_ = pNode;
			}

			if(pNode->pPrevY())
				pNode->pPrevY()->pNextY(pNode->pNextY());

			if(pNode->pNextY())
				pNode->pNextY()->pPrevY(pNode->pPrevY());

			pNode->pPrevY(pPreNode);
			pNode->pNextY(pCurrNode);
		}
		else
		{
			CoordinateNode* pNextNode = pCurrNode->pNextY();
			if(pNextNode != pNode)
			{
				pCurrNode->pNextY(pNode);
				if(pNextNode)
					pNextNode->pPrevY(pNode);

				if(pNode->pPrevY())
					pNode->pPrevY()->pNextY(pNode->pNextY());

				if(pNode->pNextY())
				{
					pNode->pNextY()->pPrevY(pNode->pPrevY());
				
					if(pNode == first_y_coordinateNode_)
						first_y_coordinateNode_ = pNode->pNextY();
				}

				pNode->pPrevY(pCurrNode);
				pNode->pNextY(pNextNode);
			}
		}
	}
}

//-------------------------------------------------------------------------------------
void CoordinateSystem::moveNodeZ(CoordinateNode* pNode, float pz, CoordinateNode* pCurrNode)
{
	if(pCurrNode != NULL)
	{
		if(pCurrNode->z() > pz)
		{
			CoordinateNode* pPreNode = pCurrNode->pPrevZ();
			pCurrNode->pPrevZ(pNode);
			if(pPreNode)
			{
				pPreNode->pNextZ(pNode);
				if(pNode == first_z_coordinateNode_ && pNode->pNextZ())
					first_z_coordinateNode_ = pNode->pNextZ();
			}
			else
			{
				first_z_coordinateNode_ = pNode;
			}

			if(pNode->pPrevZ())
				pNode->pPrevZ()->pNextZ(pNode->pNextZ());

			if(pNode->pNextZ())
				pNode->pNextZ()->pPrevZ(pNode->pPrevZ());

			pNode->pPrevZ(pPreNode);
			pNode->pNextZ(pCurrNode);
		}
		else
		{
			CoordinateNode* pNextNode = pCurrNode->pNextZ();
			if(pNextNode != pNode)
			{
				pCurrNode->pNextZ(pNode);
				if(pNextNode)
					pNextNode->pPrevZ(pNode);

				if(pNode->pPrevZ())
					pNode->pPrevZ()->pNextZ(pNode->pNextZ());

				if(pNode->pNextZ())
				{
					pNode->pNextZ()->pPrevZ(pNode->pPrevZ());
				
					if(pNode == first_z_coordinateNode_)
						first_z_coordinateNode_ = pNode->pNextZ();
				}

				pNode->pPrevZ(pCurrNode);
				pNode->pNextZ(pNextNode);
			}
		}
	}
}

//-------------------------------------------------------------------------------------
void CoordinateSystem::update(CoordinateNode* pNode)
{
	AUTO_SCOPED_PROFILE("coordinateSystemUpdates");

	//KBE_ASSERT(!(pNode->flags() & COORDINATE_NODE_FLAG_REMOVED));

#ifdef DEBUG_COORDINATE_SYSTEM
	DEBUG_MSG(fmt::format("CoordinateSystem::update enter:[{:p}]:  ({}  {}  {})\n", (void*)pNode, pNode->xx(), pNode->yy(), pNode->zz()));
#endif

	pNode->flags(pNode->flags() | COORDINATE_NODE_FLAG_PENDING);
	++updating_;

	if(pNode->xx() != pNode->old_xx())
	{
		while(true)
		{
			CoordinateNode* pCurrNode = pNode->pPrevX();
			while(pCurrNode && pCurrNode != pNode && pCurrNode->x() > pNode->xx())
			{
				pNode->x(pCurrNode->x());

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update start: [-X] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				// 先把节点移动过去
				moveNodeX(pNode, pNode->xx(), pCurrNode);

				if((pNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update1: [-X] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif
					
					pCurrNode->onNodePassX(pNode, true);
				}

				if ((pCurrNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update2: [-X] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif
					
					pNode->onNodePassX(pCurrNode, false);
				}
			
				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update end: [-X] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif

				if(pCurrNode->pPrevX() == NULL)
					break;

				pCurrNode = pCurrNode->pPrevX();
			}

			pCurrNode = pNode->pNextX();
			while(pCurrNode && pCurrNode != pNode && pCurrNode->x() < pNode->xx())
			{
				pNode->x(pCurrNode->x());

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update start: [+X] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif

				// 先把节点移动过去
				moveNodeX(pNode, pNode->xx(), pCurrNode);

				if((pNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update1: [+X] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif

					pCurrNode->onNodePassX(pNode, true);
				}

				if((pCurrNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update2: [+X] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif
					
					pNode->onNodePassX(pCurrNode, false);
				}

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update end: [+X] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				if(pCurrNode->pNextX() == NULL)
					break;

				pCurrNode = pCurrNode->pNextX();
			}

			if((pNode->pPrevX() == NULL || (pNode->xx() >= pNode->pPrevX()->x())) && 
				(pNode->pNextX() == NULL || (pNode->xx() <= pNode->pNextX()->x())))
			{
				pNode->x(pNode->xx());
				break;
			}
		}
	}

	if(CoordinateSystem::hasY && pNode->yy() != pNode->old_yy())
	{
		while(true)
		{
			CoordinateNode* pCurrNode = pNode->pPrevY();
			while(pCurrNode && pCurrNode != pNode && pCurrNode->y() > pNode->yy())
			{
				pNode->y(pCurrNode->y());

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update start: [-Y] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif

				// 先把节点移动过去
				moveNodeY(pNode, pNode->yy(), pCurrNode);

				if((pNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update1: [-Y] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif

					pCurrNode->onNodePassY(pNode, true);
				}

				if((pCurrNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update2: [-Y] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif

					pNode->onNodePassY(pCurrNode, false);
				}

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update end: [-Y] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				if(pCurrNode->pPrevY() == NULL)
					break;

				pCurrNode = pCurrNode->pPrevY();
			}

			pCurrNode = pNode->pNextY();
			while(pCurrNode && pCurrNode != pNode && pCurrNode->y() < pNode->yy())
			{
				pNode->y(pCurrNode->y());

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update start: [+Y] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
					
				// 先把节点移动过去
				moveNodeY(pNode, pNode->yy(), pCurrNode);

				if((pNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update1: [+Y] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif

					pCurrNode->onNodePassY(pNode, true);
				}

				if((pCurrNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update2: [+Y] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif
					
					pNode->onNodePassY(pCurrNode, false);
				}

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update end: [+Y] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				if(pCurrNode->pNextY() == NULL)
					break;

				pCurrNode = pCurrNode->pNextY();
			}

			if((pNode->pPrevY() == NULL || (pNode->yy() >= pNode->pPrevY()->y())) && 
				(pNode->pNextY() == NULL || (pNode->yy() <= pNode->pNextY()->y())))
			{
				pNode->y(pNode->yy());
				break;
			}
		}
	}

	if(pNode->zz() != pNode->old_zz())
	{
		while(true)
		{
			CoordinateNode* pCurrNode = pNode->pPrevZ();
			while(pCurrNode && pCurrNode != pNode && pCurrNode->z() > pNode->zz())
			{
				pNode->z(pCurrNode->z());

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update start: [-Z] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				// 先把节点移动过去
				moveNodeZ(pNode, pNode->zz(), pCurrNode);

				if((pNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update1: [-Z] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif
					
					pCurrNode->onNodePassZ(pNode, true);
				}

				if ((pCurrNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update2: [-Z] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif

					pNode->onNodePassZ(pCurrNode, false);
				}
				
				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update end: [-Z] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				if(pCurrNode->pPrevZ() == NULL)
					break;

				pCurrNode = pCurrNode->pPrevZ();
			}

			pCurrNode = pNode->pNextZ();
			while(pCurrNode && pCurrNode != pNode && pCurrNode->z() < pNode->zz())
			{
				pNode->z(pCurrNode->z());

				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update start: [+Z] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				// 先把节点移动过去
				moveNodeZ(pNode, pNode->zz(), pCurrNode);

				if((pNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update:1 [+Z] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif

					pCurrNode->onNodePassZ(pNode, true);
				}

				if((pCurrNode->flags() & COORDINATE_NODE_FLAG_HIDE_OR_REMOVED) <= 0)
				{
					#ifdef DEBUG_COORDINATE_SYSTEM
						DEBUG_MSG(fmt::format("CoordinateSystem::update:2 [+Z] ({}), passNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
					#endif

					pNode->onNodePassZ(pCurrNode, false);
				}
				
				#ifdef DEBUG_COORDINATE_SYSTEM
					DEBUG_MSG(fmt::format("CoordinateSystem::update end: [+Z] ({}), pCurrNode=>({})\n", pNode->c_str(), pCurrNode->c_str()));
				#endif
				
				if(pCurrNode->pNextZ() == NULL)
					break;

				pCurrNode = pCurrNode->pNextZ();
			}

			if((pNode->pPrevZ() == NULL || (pNode->zz() >= pNode->pPrevZ()->z())) && 
				(pNode->pNextZ() == NULL || (pNode->zz() <= pNode->pNextZ()->z())))
			{
				pNode->z(pNode->zz());
				break;
			}
		}
	}


	pNode->resetOld();
	pNode->flags(pNode->flags() & ~COORDINATE_NODE_FLAG_PENDING);
	--updating_;

	if(updating_ == 0)
		removeDelNodes();

#ifdef DEBUG_COORDINATE_SYSTEM
		DEBUG_MSG(fmt::format("CoordinateSystem::debugX[ x ]:[{:p}]\n", (void*)pNode));
		first_x_coordinateNode_->debugX();
		DEBUG_MSG(fmt::format("CoordinateSystem::debugY[ y ]:[{:p}]\n", (void*)pNode));
		if (first_y_coordinateNode_)first_y_coordinateNode_->debugY();
		DEBUG_MSG(fmt::format("CoordinateSystem::debugZ[ z ]:[{:p}]\n", (void*)pNode));
		first_z_coordinateNode_->debugZ();
#endif
}

//-------------------------------------------------------------------------------------
void CoordinateSystem::releaseNodes()
{
	std::list<CoordinateNode*>::iterator iter = releases_.begin();
	for (; iter != releases_.end(); ++iter)
	{
		delete (*iter);
	}
	releases_.clear();
}

//-------------------------------------------------------------------------------------
}
