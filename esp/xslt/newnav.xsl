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
        <script language="javascript">
            <xsl:text disable-output-escaping="yes"><![CDATA[
                $(document).ready(function() {
                    $('.dropdown').hover(
                        function () {
                            $(this).addClass("dropdown_active");
                        },
                        function () {
                            $(this).removeClass("dropdown_active");
                        }
                    );

                    $.history.init(function(hash) {
                        if (hash == "")
                        {
                            gotoURL("?main");
                        }
                        else
                        {
                            gotoURL(hash);
                        }
                    });
                });
            ]]></xsl:text>
        </script>

        <xsl:apply-templates select="*"/>
        <!--li class="dropdown">
            <xsl:attribute name="id">
                WsECL
            </xsl:attribute>
            <span>
                WsECL
            </span>
            <ul>
                <li>
                    <span>
                        <xsl:attribute name="onclick">
                            return gotoURL('/esp/navdata?root=true')
                        </xsl:attribute>
                        WsECL2.0
                    </span>
                </li>
            </ul>
        </li-->
    </xsl:template>

    <xsl:template match="Folder">
        <li class="dropdown">
            <xsl:attribute name="id">
                <xsl:value-of select="@name"/>
            </xsl:attribute>
            <span>
                <xsl:value-of select="@name"/>
            </span>
            |
            <ul>
            <xsl:apply-templates select="*"/>
            </ul>
        </li>
    </xsl:template>

    <xsl:template match="Link">
        <li>
            <a>
                <xsl:if test="@path">
                    <xsl:attribute name="href">javascript:gotoURL('<xsl:value-of select="@path"/>')</xsl:attribute>
                </xsl:if>
                <xsl:if test="@tooltip">
                    <xsl:attribute name="alt">
                        <xsl:value-of select="@tooltip"/>
                    </xsl:attribute>
                </xsl:if>
                <xsl:value-of select="@name"/>
            </a>
        </li>
    </xsl:template>

</xsl:stylesheet>
