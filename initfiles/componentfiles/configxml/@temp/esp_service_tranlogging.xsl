<?xml version="1.0" encoding="UTF-8"?>
<!--
################################################################################
#    HPCC SYSTEMS software Copyright (C) 2019 HPCC Systems®.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
################################################################################
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xml:space="default" xmlns:seisint="http://seisint.com"  xmlns:set="http://exslt.org/sets" exclude-result-prefixes="seisint set">
    <xsl:import href="esp_service.xsl"/>
    <xsl:import href="logging_agent.xsl"/>

    <xsl:template match="EspService">
        <xsl:param name="bindingNode"/>
        <xsl:param name="authNode"/>

        <xsl:variable name="serviceType" select="'ws_tranlogging'"/>
        <xsl:variable name="serviceName" select="@name"/>
        <xsl:variable name="bindName" select=" $bindingNode/@name"/>
        <xsl:variable name="bindType" select="'ws_tranloggingSoapBinding'"/>
        <xsl:variable name="servicePlugin">
            <xsl:call-template name="defineServicePlugin">
                <xsl:with-param name="plugin" select="Properties/@plugin"/>
            </xsl:call-template>
        </xsl:variable>
        <EspService name="{$serviceName}" type="{$serviceType}" plugin="{$servicePlugin}" >
            <xsl:if test="string(@tankFile) != ''">
                <TankFile><xsl:value-of select="@tankFile"/></TankFile>
            </xsl:if>
            <xsl:for-each select="ESPLoggingAgent">
                <xsl:variable name="agentName" select="@ESPLoggingAgent"/>
                <xsl:variable name="agentNode" select="/Environment/Software/ESPLoggingAgent[@name=$agentName]"/>
                <xsl:call-template name="ESPLoggingAgent">
                    <xsl:with-param name="agentName" select="$agentName"/>
                    <xsl:with-param name="agentNode" select="$agentNode"/>
                </xsl:call-template>
            </xsl:for-each>
        </EspService>

        <EspBinding name="{$bindName}" service="{$serviceName}" protocol="{$bindingNode/@protocol}" type="{$bindType}" plugin="{$servicePlugin}" netAddress="0.0.0.0" port="{$bindingNode/@port}" defaultBinding="true">
            <xsl:call-template name="bindAuthentication">
                <xsl:with-param name="bindingNode" select="$bindingNode"/>
                <xsl:with-param name="authMethod" select="$authNode/@method"/>
                <xsl:with-param name="service" select="Properties/@type"/>
            </xsl:call-template>
        </EspBinding>
    </xsl:template>
</xsl:stylesheet>
