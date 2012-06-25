<?xml version="1.0" encoding="UTF-8"?>
<!--

    Copyright (C) <2010>  <LexisNexis Risk Data Management Inc.>

    All rights reserved. This program is NOT PRESENTLY free software: you can NOT redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
    <xsl:output method="html"/>
    <xsl:template match="/">
        <html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
            <head>
                <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
                <title>User Log On</title>
                <script type="text/javascript" language="javascript">
                    <xsl:text disable-output-escaping="yes"><![CDATA[
                        function checkfields(f)
                        {
                            if(f.username.value != '' && f.password.value != '')
                            {
                                f.LogOn.disabled=false;
                            }
                            else
                            {
                                f.LogOn.disabled=true;
                            }
                        }
                        function onLoad()
                        {
                            document.getElementById("username").focus();
                        }
                    ]]></xsl:text>
                </script>
            </head>
            <body class="yui-skin-sam" onload="onLoad()">
                <p align="left" />
                <xsl:apply-templates/>
            </body>
        </html>
    </xsl:template>
    
    <xsl:template match="UserLogOn">
        <h3>
            <xsl:choose>
                <xsl:when test="string-length(Message)">
                    <xsl:value-of select="Message"/>
                </xsl:when>
                <xsl:otherwise>
                    Please Log on.
                </xsl:otherwise>
            </xsl:choose>
            <br/>
        </h3>
        <form id="user_input_form" name="user_input_form" method="GET" action="/">
            <input type="hidden" id="OriginalHTTPPath" name="OriginalHTTPPath" value="{OriginalHTTPPath}"/>
            <input type="hidden" id="OriginalQueryString" name="OriginalQueryString" value="{OriginalQueryString}"/>
            <input type="hidden" id="ErrorCode" name="ErrorCode" value="{ErrorCode}"/>
            <table>
                <tr>
                    <td>
                        <b>Username: </b>
                    </td>
                    <td>
                        <input type="text" id="username" name="username" size="20" onKeyPress="checkfields(this.form)"/>
                    </td>
                </tr>
                <tr>
                    <td>
                        <b>Password: </b>
                    </td>
                    <td>
                        <input type="password" name="password" size="20" value="" onKeyPress="checkfields(this.form)" onChange="checkfields(this.form)"/>
                    </td>
                </tr>
                <tr>
                    <td/>
                    <td>
                        <input type="submit" id="LogOn" name="LogOn" value="Log on" disabled="true"/>
                    </td>
                </tr>
            </table>
        </form>
    </xsl:template>
</xsl:stylesheet>
