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

<!DOCTYPE xsl:stylesheet [
<!ENTITY nbsp "&#160;">
]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
    <xsl:output method="html"/>
    
    <xsl:template match="EspNavigationData">
        <input type="hidden" id="applicationName" name="applicationName" value="{/EspNavigationData/@appName}"/>
        <input type="hidden" id="startPage" name="startPage" value="{/EspNavigationData/@start_page}"/>
        <input type="hidden" id="accordionCount" name="accordionCount" value="{count(/EspNavigationData/Folder)}"/>
        <div class="nav-accordion">
            <div id="nav-accordion-div" class="yui-cms-accordion Persistent slow">
                <xsl:apply-templates select="*"/>
            </div>
        </div>
    </xsl:template>
    
    <xsl:template match="Folder">
        <xsl:param name="layer" select="1"/>
        <xsl:choose>
            <xsl:when test="$layer &lt; 2">
                <div class="yui-cms-item yui-panel selected">
                    <xsl:attribute name="class">
                        <xsl:choose>
                            <xsl:when test="position() &lt; 2">yui-cms-item yui-panel selected</xsl:when>
                            <xsl:otherwise>yui-cms-item yui-panel</xsl:otherwise>
                        </xsl:choose>
                    </xsl:attribute>
                    <div class="hd">
                        <xsl:attribute name="id">
                            <xsl:text>AccordionHeader</xsl:text>
                            <xsl:value-of select="position()"/>
                        </xsl:attribute>
                        <xsl:value-of select="@name"/>
                    </div>
                    <div class="bd">
                        <xsl:attribute name="id">
                            <xsl:text>AccordionBody</xsl:text>
                            <xsl:value-of select="position()"/>
                        </xsl:attribute>
                        <div class="fixed">
                            <div class="navtree">
                                <xsl:attribute name="id">
                                    <xsl:text>AccordionContent</xsl:text>
                                    <xsl:value-of select="position()"/>
                                </xsl:attribute>
                                <ul>
                                    <xsl:for-each select=".">
                                        <xsl:apply-templates select="*">
                                            <xsl:with-param name="layer" select="2"/>
                                        </xsl:apply-templates>
                                    </xsl:for-each>
                                </ul>
                            </div>
                        </div>
                    </div>
                    <div class="actions">
                        <!--a href="#" class="accordionToggleItem">&nbsp;</a-->
                        <a href="#" class="accordionToggleItem"></a>
                    </div>
                </div>
            </xsl:when>
            <xsl:otherwise>
                <li>
                    <xsl:value-of select="@name"/>
                    <ul>
                        <xsl:for-each select=".">
                            <xsl:apply-templates select="*">
                                <xsl:with-param name="layer" select="2"/>
                            </xsl:apply-templates>
                        </xsl:for-each>
                    </ul>
                </li>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="Link">
        <li>
            <xsl:if test="@tooltip">
                <xsl:attribute name="title">
                    <xsl:value-of select="@tooltip"/>
                </xsl:attribute>
            </xsl:if>
            <a>
                <xsl:if test="@path">
                    <xsl:attribute name="href">javascript:updateContent('center-frame','<xsl:value-of select="@path"/>')</xsl:attribute>
                </xsl:if>
                <!--img border="0" src="/esp/files/img/page.gif"/-->
                <u>
                    <xsl:value-of select="@name"/>
                </u>
            </a>
        </li>
    </xsl:template>

</xsl:stylesheet>
