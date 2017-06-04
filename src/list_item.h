/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef LISTITEM_H
#define LISTITEM_H

#include <QObject>
#include <QPainter>

class ListItem : public QObject
{
    Q_OBJECT
    
public:
    ListItem();
    
    /*
     * The interface function that used to compare the two ListItem
     * The ListView requires this interface to keep the selected items unchanged when refreshed
     * 
     * @item any subclass of ListItem, you can use static_cast in implementation to access any attribute to compare two items
     * @return return true if two items have same attribute, the compare method implement by subclass of ListItem
     */
    virtual bool sameAs(ListItem *item)=0;
    
    /* 
     * The interface function that used to draw background of ListItem.
     * Such as background and selected effect.
     * 
     * @rect row corresponding to the drawing of the rectangular area
     * @painter the painter used to draw anything you want
     * @index the index of ListItem, you can draw different rows effect based on the index, such as the zebra crossing
     * @isSelect current item is selected, you can draw selected effect under content when isSelect is true
     */
    virtual void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect)=0;
    
    /* 
     * The interface function that used to draw foreground of ListItem.
     * 
     * @rect column corresponding to the drawing of the rectangular area
     * @painter the painter used to draw anything you want
     * @column the column of ListItem, you can draw different column content based on the column index
     * @index the index of ListItem, you can draw different rows effect based on the index, such as the zebra crossing
     * @isSelect current item is selected, you can draw selected effect under content when isSelect is true
     */
    virtual void drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect)=0;
};

#endif
