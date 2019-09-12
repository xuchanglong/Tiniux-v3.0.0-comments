/**********************************************************************************************************
TINIUX - A tiny and efficient embedded real time operating system (RTOS)
Copyright (C) SenseRate.com All rights reserved.
http://www.tiniux.org -- Documentation, latest information, license and contact details.
http://www.tiniux.com -- Commercial support, development, porting, licensing and training services.
--------------------------------------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met: 
1. Redistributions of source code must retain the above copyright notice, this list of 
conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice, this list 
of conditions and the following disclaimer in the documentation and/or other materials 
provided with the distribution. 
3. Neither the name of the copyright holder nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior written 
permission. 
--------------------------------------------------------------------------------------------------------
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
--------------------------------------------------------------------------------------------------------
 Notice of Export Control Law 
--------------------------------------------------------------------------------------------------------
 TINIUX may be subject to applicable export control laws and regulations, which might 
 include those applicable to TINIUX of U.S. and the country in which you are located. 
 Import, export and usage of TINIUX in any manner by you shall be in compliance with such 
 applicable export control laws and regulations. 
***********************************************************************************************************/

#include "OSList.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
Function    : OSListInit 
Description : Initialise a list. 
Input       : ptList -- Pointer of the list to be initialised.
Output      : None 
Return      : None 
*****************************************************************************/
//链表为空，初始化内容如下：
//tNilItem
//uxNumberOfItems
//ptIndex
void OSListInit( tOSList_t * const ptList )
{
	//链表当前节点指向哨兵节点。
    ptList->ptIndex = ( tOSListItem_t * ) &( ptList->tNilItem );

	//哨兵节点的uxItemValue的值为最大值时，标志该链表已经初始化。
    ptList->tNilItem.uxItemValue = OSPEND_FOREVER_VALUE;

	//由于链表除了哨兵节点没有其他节点，故哨兵节点的ptNext和ptPrevious都指向自身。
    ptList->tNilItem.ptNext = ( tOSListItem_t * ) &( ptList->tNilItem );
    ptList->tNilItem.ptPrevious = ( tOSListItem_t * ) &( ptList->tNilItem );
	//哨兵节点的pvHoler（内容）和pvList不使用，故始终为OS_NULL
    ptList->tNilItem.pvHolder = OS_NULL; 
    ptList->tNilItem.pvList = OS_NULL; 
	//节点个数为0
    ptList->uxNumberOfItems = ( uOSBase_t ) 0U;
}

/*****************************************************************************
Function    : OSListItemInitialise 
Description : Initialise a list item. 
Input       : ptListItem -- Pointer of the list item to be initialised.
Output      : None 
Return      : None 
*****************************************************************************/
void OSListItemInitialise( tOSListItem_t * const ptListItem )
{
    //初始化节点的方法就是将记录该节点所属的链表的位置清零。
    ptListItem->pvList = OS_NULL;
}

/*****************************************************************************
Function    : OSListInsertItemToEnd 
Description : Insert a new list item into ptList, but rather than sort the list,
              makes the new list item the last item. 
Input       : ptList -- Pointer of the list to be inserted.
              ptNewListItem -- Pointer of a new list item.
Output      : None 
Return      : None 
*****************************************************************************/
void OSListInsertItemToEnd( tOSList_t * const ptList, tOSListItem_t * const ptNewListItem )
{	//临时保存当前正在使用的节点
    tOSListItem_t * const ptIndex = ptList->ptIndex;
	//将新插入的节点放入当前正在使用的节点的前面	
    ptNewListItem->ptNext = ptIndex;
    ptNewListItem->ptPrevious = ptIndex->ptPrevious;
    ptIndex->ptPrevious->ptNext = ptNewListItem;
    ptIndex->ptPrevious = ptNewListItem;

    //标记该节点已经纳入该链表中
    ptNewListItem->pvList = ( void * ) ptList;
	//增加节点数量
    ( ptList->uxNumberOfItems )++;
}

/*****************************************************************************
Function    : OSListInsertItem 
Description : Insert the new list item into the list, sorted in uxItemValue order. 
Input       : ptList -- Pointer of the list to be inserted.
              ptNewListItem -- Pointer of a new list item.
Output      : None 
Return      : None 
*****************************************************************************/
//插入一个新节点。
//在链表中，节点按照自身的uxItemValue值从小到大排列。
//插入节点时按照顺序插入，若两个节点的uxItemValue相同，
//则新节点放在该节点的后面。
void OSListInsertItem( tOSList_t * const ptList, tOSListItem_t * const ptNewListItem )
{
    tOSListItem_t *ptIterator = OS_NULL;
    const uOSTick_t uxValueOfInsertion = ptNewListItem->uxItemValue;

	//若插入的节点中的值等于OSPEND_FOREVER_VALUE，说明其是最大值，直接作为链表尾端节点。
    if( uxValueOfInsertion == OSPEND_FOREVER_VALUE )
    {
        ptIterator = ptList->tNilItem.ptPrevious;
    }
    else
    {
        for( ptIterator = ( tOSListItem_t * ) &( ptList->tNilItem ); ptIterator->ptNext->uxItemValue <= uxValueOfInsertion; ptIterator = ptIterator->ptNext )
        {
            //什么也不做，仅仅是找到带插入的节点在链表的位置。
        }
    }

    ptNewListItem->ptNext = ptIterator->ptNext;
    ptNewListItem->ptNext->ptPrevious = ptNewListItem;
    ptNewListItem->ptPrevious = ptIterator;
    ptIterator->ptNext = ptNewListItem;

    ptNewListItem->pvList = ( void * ) ptList;

    ( ptList->uxNumberOfItems )++;
}

/*****************************************************************************
Function    : OSListRemoveItem 
Description : Remove an item from list. 
Input       : ptItemToRemove -- Pointer of a the list item to be removed.
Output      : None 
Return      : None 
*****************************************************************************/
//删除该节点。
//该节点前一个节点连接其后一个节点。
uOSBase_t OSListRemoveItem( tOSListItem_t * const ptItemToRemove )
{
    tOSList_t * const ptList = ( tOSList_t * ) ptItemToRemove->pvList;
    tOSListItem_t * ptListItemTemp = OS_NULL;
    
    ptListItemTemp = ptItemToRemove->ptPrevious;
    ptItemToRemove->ptNext->ptPrevious = ptListItemTemp;
    ptListItemTemp = ptItemToRemove->ptNext;
    ptItemToRemove->ptPrevious->ptNext = ptListItemTemp;

    //更新当前节点
    if( ptList->ptIndex == ptItemToRemove )
    {
        ptList->ptIndex = ptItemToRemove->ptPrevious;
    }

    ptItemToRemove->pvList = OS_NULL;
    ( ptList->uxNumberOfItems )--;

    return ptList->uxNumberOfItems;
}

#ifdef __cplusplus
}
#endif

