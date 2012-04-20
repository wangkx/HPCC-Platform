<?xml version="1.0" encoding="UTF-8"?>
<!--
##############################################################################
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
##############################################################################
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
    <xsl:output method="html" omit-xml-declaration="yes"/>
    <xsl:template match="EspApplicationFrame">
        <html>
            <head>
                <title><xsl:value-of select="@title"/><xsl:if test="@title!=''"> - </xsl:if>Enterprise Services Platform</title>
                <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui/build/fonts/fonts-min.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui/build/reset-fonts-grids/reset-fonts-grids.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui/build/resize/assets/skins/sam/resize.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui/build/layout/assets/skins/sam/layout.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui/build/button/assets/skins/sam/button.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui/build/container/assets/skins/sam/container.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui/build/treeview/assets/skins/sam/treeview.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui_test/caridy-bubbling-library-edd810c/build/accordion/assets/accordion.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui_test/css/tree.css"/>
                <link rel="stylesheet" type="text/css" href="/esp/files/css/espdefault.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/css/eclwatch.css" />
                <link rel="stylesheet" type="text/css" href="/esp/files/yui_test/css/eclwatch_yui.css"/>
                <link rel="shortcut icon" href="/esp/files/img/favicon.ico" />

                <script type="text/javascript" src="/esp/files/yui/build/yahoo/yahoo-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/event/event-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/dom/dom-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/element/element-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/dragdrop/dragdrop-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/resize/resize-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/animation/animation-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/layout/layout-min.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/yahoo-dom-event/yahoo-dom-event.js"></script>
                <script type="text/javascript" src="/esp/files/yui/build/treeview/treeview-min.js"></script>

                <script type="text/javascript" src="/esp/files/yui/build/utilities/utilities.js"></script>
                <script type="text/javascript" src="/esp/files/yui_test/caridy-bubbling-library-edd810c/build/bubbling/bubbling.js"></script>
                <script type="text/javascript" src="/esp/files/yui_test/caridy-bubbling-library-edd810c/build/accordion/accordion.js"></script>
                <script type="text/javascript">
                    var layout;
                    var eclTree;
                    var passwordDays='<xsl:value-of select="@passwordDays"/>';
                    <xsl:text disable-output-escaping="yes"><![CDATA[
                        var passwordCookie = "ESP Password Cookie";
                        function areCookiesEnabled() {
	                        var cookieEnabled = (navigator.cookieEnabled) ? true : false;
	                        if (typeof navigator.cookieEnabled == "undefined" && !cookieEnabled) {
		                        document.cookie="testcookie";
		                        cookieEnabled = (document.cookie.indexOf("testcookie") != -1) ? true : false;
	                        }
	                        return (cookieEnabled);
                        }
                        function updatePassword() {
                            var dt = new Date();
                            dt.setDate(dt.getDate() + 1);
                            var exdate = new Date(dt.getFullYear(), dt.getMonth(), dt.getDate());
                            document.cookie = passwordCookie + "=1; expires=" + exdate.toUTCString() + "; path=/";

                            var msg = 'Your password will be expired in ' + passwordDays + ' day(s). Do you want to change it now?'
                            if (confirm(msg)) {
                                var mywindow = window.open('/esp/updatepasswordinput', 'UpdatePassword', 'toolbar=0,location=no,titlebar=0,status=0,directories=0,menubar=0', true);
                                if (mywindow.opener == null)
                                    mywindow.opener = window;
                                mywindow.focus();
                            }
                            return true;
                        }

                        function updateContent(frameId, url) {
                            var iframe = document.getElementById(frameId);
                            if (iframe != null) {
                                if (frameId == 'center-frame') {
                                    var parent = layout.getUnitByPosition('center');
                                    iframe.style.width = (parent.getSizes().body.w - 5) + "px";
                                    iframe.style.height = (parent.getSizes().body.h - 5) + "px";
                                } else {
                                    var parent = layout.getUnitByPosition('bottom');
                                    iframe.style.width = (parent.getSizes().body.w - 10) + "px";
                                    iframe.style.height = (parent.getSizes().body.h - 10) + "px";
                                }
                                iframe.setAttribute('src', url);
                            }
                        }

                        function submitSiteSearch() {
                            var value = document.getElementById('SearchInput').value;
                            if (value.match("w") || value.match("W")) {
                                url = '/WsWorkunits/WUInfo?Wuid=' + value;
                            }
                            else if (value.match("d") || value.match("D")) {
                                url = '/FileSpray/GetDFUWorkunit?wuid=' + value;
                            }
                            else {
                                url = '/WsDfu/DFUInfo?Name=' + value;
                            }

                            updateContent('center-frame', url);
                        }

                        function showStartPage() {
                            var startPage = document.getElementById("startPage").value;
                            if (startPage != '')
                                updateContent("center-frame", startPage);
                            else
                                updateContent("center-frame", "/WsSMC/Activity");
                        }

                        (function() {
                            var Dom = YAHOO.util.Dom,
                            Event = YAHOO.util.Event;

                            //var tt, contextElements = [];
                            //var tree; //will hold our TreeView instance
                            function layoutInit() {
                                layout = new YAHOO.widget.Layout({
                                    units: [
                                        { position: 'top', height: '56px', body: 'top-header', useShim:true, gutter: '5 5 0 5' },
                                        { position: 'left', width: '210px', resize: true, body: 'page-navigation', useShim:true, gutter: '5 5 0 5', collapse: true, collapseSize: 50},
                                        { position: 'bottom', height: '500px', body: 'page-bottom', useShim:true, gutter: '5 5 0 5', resize: true, collapse: true, collapseSize: 0 },
                                        { position: 'center', height: '800px', body: 'page-center', useShim:true, gutter: '5 5 0 0', scroll: true, animate: true  }
                                    ]
                                });

                                layout.on('render', function() {
                                        layout.getUnitByPosition('left').on('close', function() {
                                        closeLeft();
                                    });

                                    layout.getUnitByPosition('bottom').on('expand', function() {
                                        updateContent("bottom-frame", "/esp/files/yui_test/docs/eclwatch_help.htm");
                                    });

                                    layout.getUnitByPosition('top').setStyle('border', 'none');
                                    layout.getUnitByPosition('bottom').collapse();
                                });

                                layout.render();
                            }

                            var treeNodeToBeExpanded;
                            var startExpend =false;
                            var updateTreeNodeCallback = {
                                success: function(o) {
                                    var treeNodes = o.responseText;
                                    //alert(o.responseText);
                                    var treeNodeList = treeNodes.split(',');
                                    if (treeNodeList.length < 1)
                                        return;

                                    eclTree.removeChildren(treeNodeToBeExpanded);
                                    for (var i = 0, len = treeNodeList.length; i < len; i++) {
                                        var tempNode = new YAHOO.widget.TextNode(treeNodeList[i], treeNodeToBeExpanded, false); 
                                        if (treeNodeList[i]!='N/A') {
                                            tempNode.href = "javascript:updateContent('center-frame', '/WsWorkunits/WUInfo?Wuid=" + treeNodeList[i] + "')";
                                            tempNode.title = "View " + treeNodeList[i] + " details";
                                        }
                                        tempNode.isLeaf = true; 
                                    }
                                    startExpend = true;
                                    treeNodeToBeExpanded.expand();
                                    startExpend = false;
                                },
                                failure: function(o) {
                                    alert("AJAX doesn't work"); //FAILURE
                                }

                            }

                            function expandTreeNode(node) {
                                if (startExpend)
                                    return true;

                                if (node.label=='Systems') {
                                    treeNodeToBeExpanded = node;
                                    YAHOO.util.Connect.asyncRequest('GET', '/WsWorkunits/WURecentWUs', updateTreeNodeCallback, null);
                                    return false;
                                }
                                else if (node.label=='Recent Workunits' && node.parent.label=='Ecl Workunits') {
                                    treeNodeToBeExpanded = node;
                                    YAHOO.util.Connect.asyncRequest('GET', '/WsWorkunits/WURecentWUs', updateTreeNodeCallback, null);
                                    return false;
                                }
                                return true;
                            }

                            function treeInit() {
                                var accordionCount = document.getElementById("accordionCount").value;
                                for (i = 1; i <= accordionCount; i++) {
                                    var name ="AccordionContent" + i;
                                    var tree1 = new YAHOO.widget.TreeView(name);
                                    if (i == 2)
                                        eclTree = tree1;

                                    tree1.subscribe("expand", function(node) {
                                            return expandTreeNode(node);
                                            // return false; // return false to cancel the expand
                                        });

                                    tree1.render();
                                }
                            }

                            function resizeAccordionPanel() {
                                //alert(layout.getUnitByPosition('center').getSizes().body.h);
                                var leftHeight = layout.getUnitByPosition('left').getSizes().body.h;
                                var hd1Height = Dom.get('AccordionHeader1').offsetHeight;
                                var bd4 = document.getElementById("AccordionBody1");
                                var accordionCount = document.getElementById("accordionCount").value;
                                accordionBdHeight =(leftHeight-accordionCount*hd1Height-5)+'px'; //gutter size: 5
                                bd4.style.height=accordionBdHeight; //also used in accordion.js
                                bd4.style.overflow='auto';
                            }

                            var navCallback = {
                                success: function(o) {
                                    document.getElementById('page-navigation').innerHTML =  o.responseText;

                                    resizeAccordionPanel();

                                    treeInit();

                                    var applicationName = document.getElementById("applicationName").value;
                                    if (applicationName != '')
                                       document.getElementById('application').innerHTML = '<p align="center"><b><font size="5">' + applicationName + '</font></b><br/>version-1234</p>';
                                    
                                    showStartPage();
                                },
                                failure: function(o) {
                                    alert("AJAX doesn't work"); //FAILURE
                                }
                            }

                            Event.onDOMReady(function() {
                                if ((passwordDays > -1) && areCookiesEnabled() && (document.cookie == '' || (document.cookie.indexOf(passwordCookie) == -1))) {
                                   updatePassword();
                                }

                                layoutInit();

                                Event.on('help-open', 'click', function(ev) {
                                    Event.stopEvent(ev);
                                    layout.getUnitByPosition('bottom').expand();
                                });

                                YAHOO.util.Connect.asyncRequest('GET', "esp/nav", navCallback, null);
                            });
                        })();
                    ]]></xsl:text>
                </script>
            </head>
            <body class="yui-skin-sam">
                <div id="top-header" style="border:none">
                    <div id="logo" onclick="return showStartPage();">
                        <a href="" name="" />
                    </div>
                    <div id="links">
                        <ul>
                            <li id="adv_search">
                                <a>
                                    <input id="SearchInput" name="SearchInput" type="text" size="20" value="Search WU or file" onclick="this.focus();this.select();"/>
                                    <input src="/esp/files/yui_test/img/btn_search.gif" type="image" name="" align="absmiddle" alt="Submit" class="search_btn" onclick="return submitSiteSearch()"/>
                                </a>
                            </li>
                            <li id="config" title="This link displays the esp.xml.">
                                <a href="javascript:updateContent('center-frame', '/main?config_');">
                                    <img border="0" src="/esp/files/yui_test/img/config.png"/>
                                </a>
                            </li>
                            <li  id="help-open" title="This link opens a help panel.">
                                <a>
                                    <input src="/esp/files/yui_test/img/help.png" type="image" name="" align="absmiddle" alt="Submit" class="search_btn"/>
                                </a>
                            </li>
                        </ul>
                    </div>
                    <div id="application">
                        EclWatch
                    </div>
                </div>
                <div id="page-bottom">
                    <iframe id="bottom-frame" src="" frameborder="0" scrolling="auto" style="height:100%;width:100%;">
                        Test
                    </iframe>
                </div>
                <div id="page-navigation">
                    <!--div class="nav-accordion">
                        <div id="nav-accordion-div" class="yui-cms-accordion Persistent slow">
                            <div class="yui-cms-item yui-panel selected">
                                <div id="AccordionHeader1" class="hd">Systems</div>
                                <div id="AccordionBody1" class="bd">
                                    <div class="fixed">
                                        <div id="systemNav" class="navtree">
                                            <ul>
                                                <li class="file" title="Display Activity on all target clusters in an environment">
                                                    <a href="javascript:updateContent('center-frame','/WsSMC/Activity');">Activity</a>
                                                </li>
                                                <li class="cluster" title="This is List 0">
                                                    Clusters
                                                    <ul>
                                                        <li class="expanded">
                                                            <span class="cluster">Target Clusters</span>
                                                            <ul>
                                                                <li title="View details about target clusters and optionally run preflight activities">
                                                                    <a href="javascript:updateContent('center-frame','/WsTopology/TpTargetClusterQuery?Type=ROOT');">All</a>
                                                                </li>
                                                                <li>
                                                                    <span class="cluster">hthor</span>
                                                                </li>
                                                                <li>
                                                                    <span class="cluster">thor</span>
                                                                </li>
                                                                <li>
                                                                    <span class="cluster">roxie</span>
                                                                </li>
                                                            </ul>
                                                        </li>
                                                        <li>
                                                            <span class="cluster">Cluster Processes</span>
                                                            <ul id="folder22">
                                                                <li title="View details about clusters and optionally run preflight activities">
                                                                    <a href="javascript:updateContent('center-frame','/WsTopology/TpClusterQuery?Type=ROOT')">All</a>
                                                                </li>
                                                                <li>
                                                                    <span class="cluster">mythor</span>
                                                                </li>
                                                                <li>
                                                                    <span class="cluster">myroxie</span>
                                                                </li>
                                                            </ul>
                                                        </li>
                                                    </ul>
                                                </li>
                                                <li>
                                                    <span class="cluster">Servers</span>
                                                    <ul>
                                                        <li title="View details about System Support Servers clusters and optionally run preflight activities">
                                                            <a href="javascript:updateContent('center-frame','/WsTopology/TpServiceQuery?Type=ALLSERVICES')">All</a>
                                                        </li>
                                                        <li>
                                                            <span class="server">Dali server</span>
                                                        </li>
                                                        <li>
                                                            <span class="cluster">ESPs</span>
                                                            <ul id="folder23">
                                                                <li>
                                                                    <span class="server">myesp</span>
                                                                </li>
                                                                <li>
                                                                    <span class="server">esp2</span>
                                                                </li>
                                                            </ul>
                                                        </li>
                                                    </ul>
                                                </li>
                                                <li>
                                                    <a href="javascript:updateContent('center-frame','/WsSMC/BrowseResources')">Browse reources</a>
                                                </li>
                                            </ul>
                                        </div>
                                    </div>
                                </div>
                                <div class="actions">
                                    <a href="#" class="accordionToggleItem"></a>
                                </div>
                            </div>
                            <div class="yui-cms-item yui-panel">
                                <div class="hd">ECL</div>
                                <div class="bd">
                                    <div class="fixed">
                                        <div id="eclNav">
                                            <ul>
                                                <li>
                                                    <span class="folder">Query Sets</span>
                                                    <ul id="folder11">
                                                        <li>
                                                            <span class="file">All</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">Query Set 1</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">Query Set 2</span>
                                                        </li>
                                                    </ul>
                                                </li>
                                                <li>
                                                    <span class="folder">Ecl Workunits</span>
                                                    <ul id="folder12">
                                                        <li>
                                                            <span class="file">All</span>
                                                        </li>
                                                        <li>
                                                            <span class="tool">Search</span>
                                                        </li>
                                                        <li class="closed">
                                                            <span class="folder">Today's</span>
                                                            <ul id="folder13">
                                                                <li>
                                                                    <span class="file">WU1</span>
                                                                </li>
                                                                <li>
                                                                    <span class="file">WU2</span>
                                                                </li>
                                                            </ul>
                                                        </li>
                                                    </ul>
                                                </li>
                                                <li>
                                                    <span class="tool">Run ECL</span>
                                                </li>
                                                <li>
                                                    <span class="tool">Schduler</span>
                                                </li>
                                            </ul>
                                        </div>
                                    </div>
                                </div>
                                <div class="actions">
                                    <a href="#" class="accordionToggleItem"></a>
                                </div>
                            </div>
                            <div class="yui-cms-item yui-panel">
                                <div class="hd">Files</div>
                                <div class="bd">
                                    <div class="fixed">
                                        <div id="fileNav">
                                            <ul>
                                                <li>
                                                    <span class="tool">Upload/download File</span>
                                                </li>
                                                <li class="closed">
                                                    <span class="folder">Drop Zones</span>
                                                    <ul id="folder31">
                                                        <li>
                                                            <span class="file">All</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">Drop Zone 1</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">Drop Zone 2</span>
                                                        </li>
                                                    </ul>
                                                </li>
                                                <li class="closed">
                                                    <span class="folder">Logical Files</span>
                                                    <ul id="folder32">
                                                        <li>
                                                            <span class="tool">Search</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">Browse</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">File View by Scope</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">File Relationships</span>
                                                        </li>
                                                        <li>
                                                            <span class="file">Space Usage</span>
                                                        </li>
                                                    </ul>
                                                </li>
                                                <li>
                                                    <span class="tool">View Data File</span>
                                                </li>
                                                <li class="closed">
                                                    <span class="folder">DFU Workunits</span>
                                                    <ul id="folder33">
                                                        <li>
                                                            <span class="file">All</span>
                                                        </li>
                                                        <li>
                                                            <span class="tool">Search</span>
                                                        </li>
                                                        <li class="closed">
                                                            <span class="folder">Today's</span>
                                                            <ul id="folder34">
                                                                <li>
                                                                    <span class="file">WU1</span>
                                                                </li>
                                                                <li>
                                                                    <span class="file">WU2</span>
                                                                </li>
                                                            </ul>
                                                        </li>
                                                    </ul>
                                                </li>
                                                <li class="closed">
                                                    <span class="folder">Actions</span>
                                                    <ul id="folder35">
                                                        <li>
                                                            <span class="tool">Spray Fixed</span>
                                                        </li>
                                                        <li>
                                                            <span class="tool">Spray CSV</span>
                                                        </li>
                                                        <li>
                                                            <span class="tool">Spray XML</span>
                                                        </li>
                                                        <li>
                                                            <span class="tool">Remote Copy</span>
                                                        </li>
                                                        <li>
                                                            <span class="tool">XRef</span>
                                                        </li>
                                                    </ul>
                                                </li>
                                            </ul>
                                        </div>
                                    </div>
                                </div>
                                <div class="actions">
                                    <a href="#" class="accordionToggleItem"></a>
                                </div>
                            </div>
                            <div class="yui-cms-item yui-panel">
                                <div class="hd">Security</div>
                                <div class="bd">
                                    <div class="fixed">
                                        <div id="securityNav">
                                            <ul>
                                                <li>
                                                    <span class="users">Users</span>
                                                </li>
                                                <li>
                                                    <span class="usergroup">Groups</span>
                                                </li>
                                                <li>
                                                    <span class="file">Permissions</span>
                                                </li>
                                            </ul>
                                        </div>
                                    </div>
                                </div>
                                <div class="actions">
                                    <a href="#" class="accordionToggleItem"></a>
                                </div>
                            </div>
                        </div>
                    </div-->
                </div>
                <div id="page-center">
                    <iframe id="center-frame" src="" frameborder="0" scrolling="auto"  style="height:100%;width:100%;">
                        Test
                    </iframe>
                </div>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
