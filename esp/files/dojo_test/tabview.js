/*##############################################################################
#    Copyright (C) 2011 HPCC Systems.
#
#    All rights reserved. This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as
#    published by the Free Software Foundation, either version 3 of the
#    License, or (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
############################################################################## */

var viewSeq = 1;
var currentTabView;
var previousTabView;
var tabViews = new Array();

function tabView(id, url, contentType, refreshInterval) {
	this.id = id;
	this.url = url;
	this.contentType = contentType;
	this.refreshInterval = refreshInterval;
}

function addTabView(id, url, contentType, refreshInterval) {
	var newTabView = new tabView(id, url, contentType, refreshInterval);
	tabViews[tabViews.length] = newTabView;
	
	return newTabView;
}

function getTabView(id) {
	var tabView;
	for (var i=0; i<tabViews.length; i++) {
		if (id == tabViews[i].id) {
			tabView = tabViews[i];
			break;
		}
	}
	
	return tabView;
}

function updateTabView(id, url, contentType, refreshInterval) {
	var tabView = getTabView(id);
	
	if (typeof(tabView) != 'undefined') {
		tabView.url = url;
		tabView.contentType = contentType;
		tabView.refreshInterval = refreshInterval;
	}
	
	return tabView;
}

function removeTabView(id) {
	for (var i=0; i<tabViews.length; i++) {
		if (id == tabViews[i].id) {
			tabViews = tabViews.splice(i,1);
			break;
		}
	}
}
