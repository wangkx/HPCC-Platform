<?xml version="1.0" encoding="UTF-8"?>

<!--
################################################################################
#    HPCC SYSTEMS software Copyright (C) 2016 HPCC Systems®.
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

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xml:space="default"
xmlns:seisint="http://seisint.com"  xmlns:set="http://exslt.org/sets" exclude-result-prefixes="seisint set">
    <xsl:import href="esp_service.xsl"/>
    <xsl:import href="logging_agent.xsl"/>

    <xsl:template match="EspService">
        <xsl:param name="authNode"/>
        <xsl:param name="bindingNode"/>

        <xsl:variable name="serviceType" select="'WsLoggingService'"/>
        <xsl:variable name="bindType" select="'loggingservice_binding'"/>
        <xsl:variable name="servicePlugin">
            <xsl:choose>
                <xsl:when test="$isLinuxInstance">ws_loggingservice</xsl:when>
                <xsl:otherwise>ws_loggingservice.dll</xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        <xsl:variable name="serviceName" select="concat('WsLoggingService', '_', @name, '_', $process)"/>
        <xsl:variable name="bindName" select="concat('WsLoggingService', '_', $bindingNode/@name, '_', $process)"/>

        <EspService name="{$serviceName}" type="{$serviceType}" plugin="{$servicePlugin}">
            <xsl:if test="string(@LoggingManager) != ''">
                <xsl:variable name="managerName" select="@LoggingManager"/>
                <xsl:variable name="managerNode" select="/Environment/Software/LoggingManager[@name=$managerName]"/>

                <xsl:if test="not($managerNode)">
                    <xsl:message terminate="yes">Logging Manager is undefined!</xsl:message>
                </xsl:if>

                <xsl:if test="not($managerNode/ESPLoggingAgent/@ESPLoggingAgent)">
                     <xsl:message terminate="yes">ESP Logging Agent is undefined for <xsl:value-of select="$managerName"/> !</xsl:message>
                </xsl:if>

                <LoggingManager name="{$managerNode/@name}">
                    <xsl:if test="string($managerNode/@FailSafe) != ''">
                        <FailSafe><xsl:value-of select="$managerNode/@FailSafe"/></FailSafe>
                    </xsl:if>
                    <xsl:if test="string($managerNode/@FailSafeLogsDir) != ''">
                        <FailSafeLogsDir><xsl:value-of select="$managerNode/@FailSafeLogsDir"/></FailSafeLogsDir>
                    </xsl:if>
                    <xsl:if test="string($managerNode/@SafeRolloverThreshold) != ''">
                        <SafeRolloverThreshold><xsl:value-of select="$managerNode/@SafeRolloverThreshold"/></SafeRolloverThreshold>
                    </xsl:if>
                    <Filters>
                        <xsl:for-each select="$managerNode/Filter">
                            <Filter value="{current()/@filter}" type="{current()/@type}"/>
                        </xsl:for-each>
                    </Filters>

                    <xsl:for-each select="$managerNode/ESPLoggingAgent">
                        <xsl:variable name="agentName" select="@ESPLoggingAgent"/>
                        <xsl:variable name="cassandraLoggingAgentNode" select="/Environment/Software/CassandraLoggingAgent[@name=$agentName]"/>
                        <xsl:variable name="wsLogServiceESPAgentNode" select="/Environment/Software/WsLogServiceESPAgent[@name=$agentName]"/>
                        <xsl:choose>
                            <xsl:when test="($cassandraLoggingAgentNode)">
                                <xsl:call-template name="CassandraLoggingAgent">
                                    <xsl:with-param name="agentName" select="$agentName"/>
                                    <xsl:with-param name="agentNode" select="$cassandraLoggingAgentNode"/>
                                </xsl:call-template>
                            </xsl:when>
                            <xsl:when test="($wsLogServiceESPAgentNode)">
                                <xsl:call-template name="WsLogServiceESPAgent">
                                    <xsl:with-param name="agentName" select="$agentName"/>
                                    <xsl:with-param name="agentNode" select="$wsLogServiceESPAgentNode"/>
                                </xsl:call-template>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:message terminate="yes">ESP Logging Agent is undefined for <xsl:value-of select="$managerName"/> !</xsl:message>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:for-each>
                </LoggingManager>

            </xsl:if>
        </EspService>

        <EspBinding name="{$bindName}" service="{$serviceName}" protocol="{$bindingNode/@protocol}" type="{$bindType}" plugin="{$servicePlugin}" netAddress="0.0.0.0" port="{$bindingNode/@port}">
            <xsl:call-template name="bindAuthentication">
                <xsl:with-param name="bindingNode" select="$bindingNode"/>
                <xsl:with-param name="authMethod" select="$authNode/@method"/>
            </xsl:call-template>
        </EspBinding>
    </xsl:template>

    <!-- UTILITY templates-->
    <xsl:template name="bindAuthentication">
      <xsl:param name="authMethod"/>
      <xsl:param name="bindingNode"/>
      <xsl:choose>
         <xsl:when test="$authMethod='basic'">
            <Authenticate type="Basic" method="UserDefined">
               <xsl:for-each select="$bindingNode/Authenticate[string(@path) != '']">
                  <Location path="{@path}"/>
               </xsl:for-each>
            </Authenticate>
         </xsl:when>
         <xsl:when test="$authMethod='ldap' or $authMethod='ldaps'">
            <Authenticate method="LdapSecurity" config="ldapserver">
            <xsl:copy-of select="$bindingNode/@resourcesBasedn"/> <!--if binding has an ldap resourcebasedn specified then copy it out -->

            <xsl:for-each select="$bindingNode/Authenticate">
               <Location path="{@path}" resource="{@resource}" access="{@access}"/>
            </xsl:for-each>

            <xsl:for-each select="$bindingNode/AuthenticateFeature[@authenticate='Yes']">
               <Feature name="{@name}" path="{@path}" resource="{@resource}" required="{@access}" description="{@description}"/>
            </xsl:for-each>

            <xsl:for-each select="$bindingNode/AuthenticateSetting[@include='Yes']">
               <Setting path="{@path}" resource="{@resource}" description="{@description}"/>
            </xsl:for-each>
            </Authenticate>
         </xsl:when>
         <xsl:when test="$authMethod='secmgrPlugin'">
            <Authenticate>
            <xsl:attribute name="method">
              <xsl:value-of select="$bindingNode/@type"/>
            </xsl:attribute>
            <xsl:copy-of select="$bindingNode/@resourcesBasedn"/>

            <xsl:for-each select="$bindingNode/Authenticate">
               <Location path="{@path}" resource="{@resource}" access="{@access}"/>
            </xsl:for-each>

            <xsl:for-each select="$bindingNode/AuthenticateFeature[@authenticate='Yes']">
               <Feature name="{@name}" path="{@path}" resource="{@resource}" required="{@access}" description="{@description}"/>
            </xsl:for-each>

            <xsl:for-each select="$bindingNode/AuthenticateSetting[@include='Yes']">
               <Setting path="{@path}" resource="{@resource}" description="{@description}"/>
            </xsl:for-each>
            </Authenticate>
         </xsl:when>
      </xsl:choose>
    </xsl:template>

</xsl:stylesheet>
