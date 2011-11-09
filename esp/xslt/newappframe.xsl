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
            <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
            <title>HPCC Systems Management Studio</title>
            <link rel="stylesheet" type="text/css" href="/esp/files/css/eclwatchheaderfooter.css" />
            <script type="text/javascript" src="/esp/files/jquery/jquery.js"/>
            <script type="text/javascript" src="/esp/files/jquery/jquery.history.js"/>
            <script type="text/javascript" src="/esp/files/scripts/espdefault.js"/>
        </head>
        <script language="JavaScript1.2">
            <xsl:text disable-output-escaping="yes"><![CDATA[
                $(document).ready(function() {
                    $.ajax({
                        url: "/esp/nav",

                        //on success, set the html to the responsetext
                        success: function(data){
                            $("#primary_nav").html(data);
                        }
                    });
                });

                function submit_site_search() {
                    var r=String(document.getElementById("SearchInput").value).match(/[Ww](\d{8}-\d{6}(-\d+)?)/);
                    if(r && r[1]) {
                        gotoURL('/WsWorkunits/WUInfo?Wuid=W'+r[1]);
                        return true;
                    }
                    alert('Wrong WUID');
                    return false;
                }
            ]]></xsl:text>
        </script>
        <body id="eclWatchBody">
            <!--div style="position: absolute; left: -99999em;">
                <a href="#page_wrapper" accesskey="s">Skip the navigation</a>
            </div-->
            <div id="eclWatchBodyContainer">
                <div id="site_background">
                        <div id="eclWatchHeader">
                            <div id="navigation">
                                <div id="eclWatchLogo">
                                    <a href="/" name="" />
                                </div>
                                <div>
                                <ul id="secondary_nav">
                                    <li id="hpcc_systems">
                                        <a href="http://hpccsystems.com/" target="_blank">HPCC Systems</a>
                                    </li>
                                    <li id="meetup">
                                        <a href="http://www.meetup.com/Big-Data-Processing-and-Analytics-LexisNexis-HPCC-Systems/" target="_blank">Meetup</a>
                                    </li>
                                    <li id="rss">
                                        <a href="http://hpccsystems.com/rss" target="_blank">RSS</a>
                                    </li>
                                    <li id="facebook">
                                        <a href="http://www.facebook.com/hpccsystems" target="_blank">Facebook</a>
                                    </li>
                                    <li id="twitter">
                                        <a href="http://twitter.com/hpccsystems" target="_blank">Twitter</a>
                                    </li>
                                    <!--li id="my_account">
                                        <a href="javascript:gotoURL('/Ws_LNAccount/UpdateUserInput')">My Account</a>
                                    </li>
                                    <li id="logout">
                                        <a href="/s/webcasts/1">Logout</a>
                                    </li>
                                    <li id="events">
                                        <a href="/s/events">Events</a>
                                    </li-->
                                </ul>
                                <!-- end secondary nav -->
                                </div>
                                <div id="site_search">
                                    <input id="SearchInput" name="SearchInput" type="text" size="20" />
                                    <input src="/esp/files/img/btn_search.gif" type="image"
                                        name="" align="absmiddle" border="0" alt="Submit" class="search_btn" onclick="return submit_site_search()"/>
                                </div>
                                <!-- end google search box -->
                                <ul id="primary_nav"/>
                            </div>
                            <!-- end sitewide nav -->
                        </div>
                        <!-- end header -->
                        <div id="banner">
                            <div>
                                <!--center style="background-color: #F6F6F6; width: 100%; align: center; text-align: center;"/-->
                            </div>
                        </div>
                        <div id="linker" />
                        <div id="page_wrapper">
                            <div id="eclWatchContent" class="clearfix">
                                <!--div id="skip_to_top">
                                    <a href="#container">Skip to top</a>
                                </div-->
                            </div>
                            <!-- end page wrapper -->
                            <div id="footer">
                                <ul id="site_links">
                                    <li>
                                        <a rel="nofollow" href="/s/pages/about_cw">About Us</a>
                                    </li>
                                    <li>
                                        <a rel="nofollow" href="/s/pages/contacts/contacts_all">Contacts</a>
                                    </li>
                                    <li>
                                        <a rel="nofollow" href="/s/help-desk">Help Desk</a>
                                    </li>
                                    <li>
                                        <a rel="nofollow" href="/s/newsletters?source=ctwnla_footer">Newsletters</a>
                                    </li>
                                    <li>
                                        <a rel="nofollow" href="/s/pages/about_policies">Privacy Policy</a>
                                    </li>
                                    <li>
                                        <a rel="nofollow" href="/s/pages/site_map">Site Map</a>
                                    </li>
                                </ul>
                                <!-- end site links -->
                                <p id="copyright">
                                    © 1999 - 2011 LexisNexis Risk Solution. All rights reserved.
                                </p>
                            </div>
                    </div>
                </div>
            </div>
        </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
