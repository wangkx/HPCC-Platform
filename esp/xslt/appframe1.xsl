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
                <link rel="stylesheet" type="text/css" href="/esp/files/yui_test/accordion.css" />
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
                <script type="text/javascript" src="/esp/files/yui_test/bubbling.js"></script>
                <script type="text/javascript" src="/esp/files/yui_test/accordion.js"></script>
                <script type="text/javascript">
                    var layout;
                    var eclTree, tree;
                    var applicationName;
                    var startPage;
                    var currentIconMode;
                    var idleTimer;
                    var idleTimeout = 300000; //Default idle time is 5 minute
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

                        function logout() {
                            document.location.href = "/WsSMC/Activity";//TODO: need a logout link to a "Thank You Page"
                        }

                        // mozilla only
                        function checkForParseError (xmlDocument) {
                            var errorNamespace = 'http://www.mozilla.org/newlayout/xml/parsererror.xml';
                            var documentElement = xmlDocument.documentElement;
                            var parseError = { errorCode : 0 };
                            if (documentElement.nodeName == 'parsererror' && documentElement.namespaceURI == errorNamespace) {
                                parseError.errorCode = 1;
                                var sourceText = documentElement.getElementsByTagNameNS(errorNamespace, 'sourcetext')[0];
                                if (sourceText != null) {
                                    parseError.srcText = sourceText.firstChild.data
                                }
                                parseError.reason = documentElement.firstChild.data;
                            }
                            return parseError;
                        }

                        function parseXmlString(xml) {
                            if (window.DOMParser)
                            {
                                parser=new DOMParser();
                                xmlDoc=parser.parseFromString(xml,"text/xml");
                                var error = checkForParseError(xmlDoc);
                                if (error.errorCode!=0)
                                {
                                    alert("XML Parse Error: " + error.reason + "\n" + error.srcText);
                                    return null;
                                }
                            }
                            else // Internet Explorer
                            {
                                xmlDoc=new ActiveXObject("Microsoft.XMLDOM");
                                xmlDoc.async=false;
                                xmlDoc.loadXML(xml);
                                if (xmlDoc.parseError != 0)
                                {
                                    alert("XML Parse Error: " + xmlDoc.parseError.reason);
                                    return null;
                                }
                            }
                            if (!xmlDoc)
                                 alert("Create xmlDoc failed! You browser is not supported yet.");

                            return xmlDoc;
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
                            if (startPage != '')
                                updateContent("center-frame", startPage);
                            else
                                updateContent("center-frame", "/WsSMC/Activity");
                        }

                        function loadNodeData(node, fnLoadComplete)  
                        {
                            var nodeLabel = encodeURI(node.label);
                            ///tree.locked = false;

                            var sUrl = "/esp/navdata?" + node.data.params;
                            var callback = {
                                success: function(oResponse) {
                                    var xmlDoc = oResponse.responseXML;

                                    node.setNodesProperty("propagateHighlightUp",false);
                                    node.setNodesProperty("propagateHighlightDown",false);

                                    var folderNodes = xmlDoc.getElementsByTagName("DynamicFolder");
                                    for(var i = 0; i < folderNodes.length; i++) {
                                        var folderNode = new YAHOO.widget.TextNode({label: folderNodes[i].getAttribute("name"), title:linkNodes[i].getAttribute("tooltip"), dynamicLoadComplete:false}, node);
                                        folderNode.data = { elementType:'DynamicFolder', params: folderNodes[i].getAttribute("params") };
                                    }
                                    var linkNodes = xmlDoc.getElementsByTagName("Link");
                                    for(var i = 0; i < linkNodes.length; i++) {
                                        var leafNode = new YAHOO.widget.TextNode({label: linkNodes[i].getAttribute("name"), title:linkNodes[i].getAttribute("tooltip"), dynamicLoadComplete:true}, node);
                                        leafNode.data = { elementType: 'Link', elementPath: linkNodes[i].getAttribute("path"), params: ''};
                                        leafNode.isLeaf = true;
                                    }

                                    oResponse.argument.fnLoadComplete();
                                },

                                failure: function(oResponse) {
                                    oResponse.argument.fnLoadComplete();
                                },

                                argument: {
                                    "node": node,
                                    "fnLoadComplete": fnLoadComplete
                                }
                                //timeout: 21000
                            };

                            YAHOO.util.Connect.asyncRequest('GET', sUrl, callback);
                        }

                        var onNodeClick = function(tNode)
                        {
                            if ((tNode.data.elementPath != null) && (tNode.data.elementPath != ''))
                                updateContent("center-frame", tNode.data.elementPath);
                        };

                        (function() {
                            var Dom = YAHOO.util.Dom,
                            Event = YAHOO.util.Event;

                            function layoutInit() {
                                layout = new YAHOO.widget.Layout({
                                    units: [
                                        { position: 'top', height: '62px', body: 'top-header', useShim:true, gutter: '5 5 0 5' },
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

                            function buildNavTreeNode(parentNode, navTreeNode) {
                                var label = navTreeNode.getAttribute("name");
                                var tooltip = navTreeNode.getAttribute("tooltip");
                                var path = navTreeNode.getAttribute("path");
                                var params = navTreeNode.getAttribute("params");

                                if (navTreeNode.nodeName=='DynamicFolder') {
                                    var paramsList = params.split(';');
                                    for(var i=0;i<paramsList.length;i++) {
                                        var paramDetails = paramsList[i].split('=');
                                        if ((paramsList.length == 2) && (paramDetails[0] == 'path'))
                                            path = paramDetails[1];
                                    }

                                    var folderNode = new YAHOO.widget.TextNode({label:label, title:tooltip, expanded:true, dynamicLoadComplete:false}, parentNode);
                                    folderNode.data = { elementType: 'DynamicFolder', elementPath: path, params: params };
                                } else if (navTreeNode.nodeName=='Folder') {
                                    var x=navTreeNode.childNodes;

                                    if (x.length < 1) {
                                        var leafNode = new YAHOO.widget.TextNode({label:label, title:tooltip, expanded:true, dynamicLoadComplete:true}, parentNode);
                                        leafNode.data = { elementType: 'Link', elementPath: path, params: ''};
                                        leafNode.isLeaf = true;
                                    } else {
                                        var folderNode = new YAHOO.widget.TextNode({label:label, title:tooltip, expanded:true, dynamicLoadComplete:true}, parentNode);
                                        folderNode.data = { elementType: 'Folder', params: ''};
//alert(label);
                                        for (var i=0;i<x.length;i++) {
                                            if ((x[i].nodeType==1) && ((x[i].nodeName=='Folder') || (x[i].nodeName=='DynamicFolder') || (x[i].nodeName=='Link')))
                                                buildNavTreeNode(folderNode, x[i]);
                                        }
                                    }
                                } else if (navTreeNode.nodeName=='Link') {
                                    var leafNode = new YAHOO.widget.TextNode({label:label, title:tooltip, expanded:true, dynamicLoadComplete:true}, parentNode);
                                    leafNode.data = { elementType: 'Link', elementPath: path, params: ''};
                                    leafNode.isLeaf = true;
                                }
                            }

                            function buildNavTree(navTree) {
                                var x=navTree.childNodes;
                                if (x.length < 1) return;
                                for (var i=0;i<x.length;i++) {
                                    if ((x[i].nodeType!=1) || (x[i].nodeName!='Folder')) 
                                        continue;

                                    var y=x[i].childNodes;
                                    if (y.length < 1) continue;

                                    var label = x[i].getAttribute("name");

                                    var tree = new YAHOO.widget.TreeView(label + "NavTree");
                                    //tree.subscribe('dblClickEvent',function(oArgs) { ImportFromTree(oArgs.Node);});

                                    // By default, trees with TextNodes will fire an event for when the label is clicked:
                                    tree.subscribe("labelClick", onNodeClick);

                                    tree.locked = false;
                                    tree.setDynamicLoad(loadNodeData, currentIconMode);
                                    tree.singleNodeHighlight=true;
                                    //tree.subscribe("clickEvent", onTreeClick);

                                    var root = tree.getRoot();
                                    for (j=0;j<y.length;j++) {
                                        if ((y[j].nodeType==1) && ((y[j].nodeName=='Folder') || (y[j].nodeName=='DynamicFolder') || (y[j].nodeName=='Link')))
                                            buildNavTreeNode(root, y[j]);
                                    }
                                    tree.render();
                                }
                            }

                            var accordionCount = 0;
                            var firstAccordionHeaderName;
                            var firstAccordionBodyName;
                            function buildNavAccordion(navTree) {
                                x=navTree.childNodes;
                                if (x.length < 1) return;

                                var html;
                                for (i=0;i<x.length;i++) {
                                    if ((x[i].nodeType!=1) || (x[i].nodeName!='Folder')) 
                                        continue;

                                    var attrs = x[i].attributes;
                                    label = attrs.getNamedItem("name").nodeValue;

                                    accordionCount++;
                                    if (accordionCount==1) {
                                        html = "<div class='nav-accordion'><div id='nav-accordion-div' class='yui-cms-accordion Persistent slow'>";
                                        html += "<div class='yui-cms-item yui-panel selected'>";
                                        firstAccordionHeaderName = 'AccordionHeader'+ label;
                                        firstAccordionBodyName = 'AccordionBody'+ label;
                                    }
                                    else {
                                        html += "<div class='yui-cms-item yui-panel'>";
                                    }
                                    
                                    html += "<div id='AccordionHeader"+ label + "' class='hd'>"+ label +"</div>";
                                    html += "<div id='AccordionBody"+ label + "' class='bd'>";
                                    html += "<div class='fixed'>";
                                    //html += x[i].nodeName;
                                    html += "<div id='"+ label + "NavTree' class='navtree'></div>";
                                    html += "</div>"; //fixed
                                    html += "</div>"; //bd

                                    html += "<div class='actions'><a href='#' class='accordionToggleItem'></a></div>";

                                    html += "</div>";
                                }
                                html += "</div></div>";

                                document.getElementById('page-navigation').innerHTML = html;
                            }

                            function resizeAccordionPanel() {
                                //alert(layout.getUnitByPosition('center').getSizes().body.h);
                                var leftHeight = layout.getUnitByPosition('left').getSizes().body.h;
                                var hd1Height = Dom.get(firstAccordionHeaderName).offsetHeight;
                                var bd4 = document.getElementById(firstAccordionBodyName);
                                //var accordionCount = document.getElementById("accordionCount").value;
                                accordionBdHeight =(leftHeight-accordionCount*hd1Height-5)+'px'; //gutter size: 5
                                bd4.style.height=accordionBdHeight; //also used in accordion.js
                                bd4.style.overflow='auto';
                            }

                            function readNavResponse(navResponse) {
                                var navTree = parseXmlString(navResponse);
                                if (!navTree) return;
                                var root = navTree.documentElement;
                                if (!root) return;

                                var attrs = root.attributes;
                                applicationName = attrs.getNamedItem("appName").nodeValue;
                                startPage = attrs.getNamedItem("startPage").nodeValue;

                                if (applicationName != '')
                                   document.getElementById('application').innerHTML = applicationName;

                                buildNavAccordion(root);
                                if (accordionCount > 0) {
                                    resizeAccordionPanel();
                                    buildNavTree(root);
                                }
                            }

                            var navCallback = {
                                success: function(o) {
                                    if (o.responseText)
                                        readNavResponse(o.responseText);
                                    else
                                        alert("No navigation data found");

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

idleTimer = setTimeout("logout()", idleTimeout);

                            });
                        })();
                    ]]></xsl:text>
                </script>
            </head>
            <body class="yui-skin-sam">
                <div id="top-header" style="border-style:solid; border-width:3px; border-color:red;">
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
                            <li class="field">
                                <input type="submit" id="Logout" name="Logout" value="Logout" onclick="return logout();"/>
                            </li>
                        </ul>
                    </div>
                    <div id="application">
                        EclWatch
                    </div>
                    <div id="version">
                        <xsl:value-of select="@buildVersion"/>
                    </div>
                </div>
                <div id="page-bottom">
                    <iframe id="bottom-frame" src="" frameborder="0" scrolling="auto" style="height:100%;width:100%;"/>
                </div>
                <div id="page-navigation">
                    <div id="page-navigation-content"/>
                </div>
                <div id="page-center">
                    <iframe id="center-frame" src="" frameborder="0" scrolling="auto"  style="height:100%;width:100%;"/>
                </div>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
