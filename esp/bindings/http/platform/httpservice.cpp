/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems®.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#pragma warning (disable : 4786)

#ifdef ESPHTTP_EXPORTS
    #define esp_http_decl DECL_EXPORT
#else
    #define esp_http_decl DECL_IMPORT
#endif
//Jlib
#include "jliball.hpp"

#include "espcontext.hpp"
#include "esphttp.hpp"

//ESP Bindings
#include "http/platform/httpsecurecontext.hpp"
#include "http/platform/httpservice.hpp"
#include "http/platform/httptransport.hpp"

#include "htmlpage.hpp"
#include "dasds.hpp"

/***************************************************************************
 *              CEspHttpServer Implementation
 ***************************************************************************/
CEspHttpServer::CEspHttpServer(ISocket& sock, bool viewConfig, int maxRequestEntityLength):m_socket(sock), m_MaxRequestEntityLength(maxRequestEntityLength)
{
    m_request.setown(new CHttpRequest(sock));
    IEspContext* ctx = createEspContext(createHttpSecureContext(m_request.get()));
    m_request->setMaxRequestEntityLength(maxRequestEntityLength);
    m_response.setown(new CHttpResponse(sock));
    m_request->setOwnContext(ctx);
    m_response->setOwnContext(LINK(ctx));
    m_viewConfig=viewConfig;
}

CEspHttpServer::CEspHttpServer(ISocket& sock, CEspApplicationPort* apport, bool viewConfig, int maxRequestEntityLength):m_socket(sock), m_MaxRequestEntityLength(maxRequestEntityLength)
{
    m_request.setown(new CHttpRequest(sock));
    IEspContext* ctx = createEspContext(createHttpSecureContext(m_request.get()));
    m_request->setMaxRequestEntityLength(maxRequestEntityLength);
    m_response.setown(new CHttpResponse(sock));
    m_request->setOwnContext(ctx);
    m_response->setOwnContext(LINK(ctx));
    m_apport = apport;
    if (apport->getDefaultBinding())
        m_defaultBinding.set(apport->getDefaultBinding()->queryBinding());
    m_viewConfig=viewConfig;
}

CEspHttpServer::~CEspHttpServer()
{
    try
    {
        IEspContext* ctx = m_request->queryContext();
        if (ctx)
        {
            //Request is logged only when there is an exception or the request time is very long.
            //If the flag of 'EspLogRequests' is set or the log level > LogNormal, the Request should
            //has been logged and it should not be logged here.
            ctx->setProcessingTime();
            if ((ctx->queryHasException() || (ctx->queryProcessingTime() > getSlowProcessingTime())) &&
                !getEspLogRequests() && (getEspLogLevel() <= LogNormal))
            {
                StringBuffer logStr;
                logStr.appendf("%s %s", m_request->queryMethod(), m_request->queryPath());

                const char* paramStr = m_request->queryParamStr();
                if (paramStr && *paramStr)
                    logStr.appendf("?%s", paramStr);

                DBGLOG("Request[%s]", logStr.str());
                if (m_request->isSoapMessage())
                {
                    StringBuffer requestStr;
                    m_request->getContent(requestStr);
                    if (requestStr.length())
                        m_request->logSOAPMessage(requestStr.str(), NULL);
                }
            }
        }

        m_request.clear();
        m_response.clear();
    }
    catch (...)
    {
        ERRLOG("In CEspHttpServer::~CEspHttpServer() -- Unknown Exception.");
    }
}

bool CEspHttpServer::rootAuth(IEspContext* ctx)
{
    if (!m_apport->rootAuthRequired())
        return true;

    bool ret=false;
    EspHttpBinding* thebinding=getBinding();
    if (thebinding)
    {
        thebinding->populateRequest(m_request.get());
        if(!thebinding->authRequired(m_request.get()) || thebinding->doAuth(ctx))
            ret=true;
        else
        {
            ISecUser *user = ctx->queryUser();
            if (user && user->getAuthenticateStatus() == AS_PASSWORD_VALID_BUT_EXPIRED)
            {
                m_response->redirect(*m_request.get(), "/esp/updatepasswordinput");
                ret = true;
            }
            else
            {
                DBGLOG("User authentication required");
                m_response->sendBasicChallenge(thebinding->getChallengeRealm(), true);
            }
        }
    }

    return ret;
}

const char* getSubServiceDesc(sub_service stype)
{
#define DEF_CASE(s) case s: return #s;
    switch(stype)
    {
    DEF_CASE(sub_serv_unknown)
    DEF_CASE(sub_serv_root)
    DEF_CASE(sub_serv_main)
    DEF_CASE(sub_serv_service)
    DEF_CASE(sub_serv_method)
    DEF_CASE(sub_serv_files)
    DEF_CASE(sub_serv_itext)
    DEF_CASE(sub_serv_iframe)
    DEF_CASE(sub_serv_content)
    DEF_CASE(sub_serv_result)
    DEF_CASE(sub_serv_index)
    DEF_CASE(sub_serv_index_redirect)
    DEF_CASE(sub_serv_form)
    DEF_CASE(sub_serv_xform)
    DEF_CASE(sub_serv_query)
    DEF_CASE(sub_serv_instant_query)
    DEF_CASE(sub_serv_soap_builder)
    DEF_CASE(sub_serv_wsdl)
    DEF_CASE(sub_serv_xsd)
    DEF_CASE(sub_serv_config)
    DEF_CASE(sub_serv_getversion)
    DEF_CASE(sub_serv_reqsamplexml)
    DEF_CASE(sub_serv_respsamplexml)
    DEF_CASE(sub_serv_file_upload)

    default: return "invalid-type";
    }
} 

static bool authenticateOptionalFailed(IEspContext& ctx, IEspHttpBinding* binding)
{
#ifdef ENABLE_NEW_SECURITY
    if (ctx.queryRequestParameters()->hasProp("internal"))
    {
        ISecUser* user = ctx.queryUser();
        if(!user || user->getStatus()==SecUserStatus_Inhouse || user->getStatus()==SecUserStatus_Unknown)
            return false;

        ERRLOG("User %s trying to access unauthorized feature: internal", user->getName() ? user->getName() : ctx.queryUserId());
        return true;
    }
    // TODO: handle binding specific optionals
#elif !defined(DISABLE_NEW_SECURITY)
#error Please include esphttp.hpp in this file.
#endif

    return false;
}

EspHttpBinding* CEspHttpServer::getBinding()
{
    EspHttpBinding* thebinding=NULL;
    int ordinality=m_apport->getBindingCount();
    if (ordinality==1)
    {
        CEspBindingEntry *entry = m_apport->queryBindingItem(0);
        thebinding = (entry) ? dynamic_cast<EspHttpBinding*>(entry->queryBinding()) : NULL;
    }
    else if (m_defaultBinding)
        thebinding=dynamic_cast<EspHttpBinding*>(m_defaultBinding.get());
    return thebinding;
}

//CORS allow headers for interoperability, we do not rely on this for security since
//that only means treating the browser as a trusted entity.  We need to be diligent and secure
//for every request whether it comes from a cross domain browser or any other source

void checkSetCORSAllowOrigin(CHttpRequest *req, CHttpResponse *resp)
{
    StringBuffer origin;
    req->getHeader("Origin", origin);
    if (origin.length())
        resp->setHeader("Access-Control-Allow-Origin", "*");
}

int CEspHttpServer::processRequest()
{
    try
    {
        if (m_request->receive(NULL)==-1) // MORE - pass in IMultiException if we want to see exceptions (which are not fatal)
            return -1;
    }
    catch(IEspHttpException* e)
    {
        m_response->sendException(e);
        e->Release();
        return 0;
    }
    catch (IException *e)
    {
        DBGLOG(e);
        e->Release();
        return 0;
    }
    catch (...)
    {
        DBGLOG("Unknown Exception - reading request [CEspHttpServer::processRequest()]");
        return 0;
    }

    try
    {
        
        EspHttpBinding* thebinding=NULL;
        
        StringBuffer method;
        m_request->getMethod(method);

        EspAuthState authState=authUnknown;
        sub_service stype=sub_serv_unknown;
        
        StringBuffer pathEx;
        StringBuffer serviceName;
        StringBuffer methodName;
        m_request->getEspPathInfo(stype, &pathEx, &serviceName, &methodName, false);
        ESPLOG(LogNormal,"sub service type: %s. parm: %s", getSubServiceDesc(stype), m_request->queryParamStr());

        m_request->updateContext();
        IEspContext* ctx = m_request->queryContext();
        ctx->setServiceName(serviceName.str());
        ctx->setHTTPMethod(method.str());
        ctx->setServiceMethod(methodName.str());

        bool isSoapPost=(stricmp(method.str(), POST_METHOD) == 0 && m_request->isSoapMessage());
#ifdef _USE_OPENLDAP
        authState = checkUserAuth();
        if ((authState == authUpdatePassword) || (authState == authFailed))
            return 0;
#endif

        if (!stricmp(method.str(), GET_METHOD))
        {
            if (stype==sub_serv_root)
            {
                return onGetApplicationFrame(m_request.get(), m_response.get(), ctx);
            }

            if (!stricmp(serviceName.str(), "esp"))
            {
                if (!methodName.length())
                    return 0;

                if (methodName.charAt(methodName.length()-1)=='_')
                    methodName.setCharAt(methodName.length()-1, 0);
                if (!stricmp(methodName.str(), "files"))
                {
                    if (!getTxSummaryResourceReq())
                        ctx->cancelTxSummary();
                    checkInitEclIdeResponse(m_request, m_response);
                    return onGetFile(m_request.get(), m_response.get(), pathEx.str());
                }
                else if (!stricmp(methodName.str(), "xslt"))
                {
                    if (!getTxSummaryResourceReq())
                        ctx->cancelTxSummary();
                    return onGetXslt(m_request.get(), m_response.get(), pathEx.str());
                }
                else if (!stricmp(methodName.str(), "body"))
                    return onGetMainWindow(m_request.get(), m_response.get());
                else if (!stricmp(methodName.str(), "frame"))
                    return onGetApplicationFrame(m_request.get(), m_response.get(), ctx);
                else if (!stricmp(methodName.str(), "titlebar"))
                    return onGetTitleBar(m_request.get(), m_response.get());
                else if (!stricmp(methodName.str(), "nav"))
                    return onGetNavWindow(m_request.get(), m_response.get());
                else if (!stricmp(methodName.str(), "navdata"))
                    return onGetDynNavData(m_request.get(), m_response.get());
                else if (!stricmp(methodName.str(), "navmenuevent"))
                    return onGetNavEvent(m_request.get(), m_response.get());
                else if (!stricmp(methodName.str(), "soapreq"))
                    return onGetBuildSoapRequest(m_request.get(), m_response.get());
            }
        }

        if(m_apport != NULL)
        {
            int ordinality=m_apport->getBindingCount();
            bool isSubService = false;
            if (ordinality>0)
            {
                if (ordinality==1)
                {
                    CEspBindingEntry *entry = m_apport->queryBindingItem(0);
                    thebinding = (entry) ? dynamic_cast<EspHttpBinding*>(entry->queryBinding()) : NULL;

                    if (thebinding && !isSoapPost && !thebinding->isValidServiceName(*ctx, serviceName.str()))
                        thebinding=NULL;
                }
                else
                {
                    EspHttpBinding* lbind=NULL;
                    for(int index=0; !thebinding && index<ordinality; index++)
                    {
                        CEspBindingEntry *entry = m_apport->queryBindingItem(index);
                        lbind = (entry) ? dynamic_cast<EspHttpBinding*>(entry->queryBinding()) : NULL;
                        if (lbind)
                        {
                            if (!thebinding && lbind->isValidServiceName(*ctx, serviceName.str()))
                            {
                                thebinding=lbind;
                                StringBuffer bindSvcName;
                                if (stricmp(serviceName, lbind->getServiceName(bindSvcName)))
                                    isSubService = true;
                            }                           
                        }
                    }
                }
                if (!thebinding && m_defaultBinding)
                    thebinding=dynamic_cast<EspHttpBinding*>(m_defaultBinding.get());
            }

            if(strieq(method.str(), OPTIONS_METHOD))
                return onOptions();

            checkSetCORSAllowOrigin(m_request, m_response);

            if (thebinding!=NULL)
            {
                if(stricmp(method.str(), POST_METHOD)==0)
                    thebinding->handleHttpPost(m_request.get(), m_response.get());
                else if(!stricmp(method.str(), GET_METHOD)) 
                {
                    if (stype==sub_serv_index_redirect)
                    {
                        StringBuffer url;
                        if (isSubService) 
                        {
                            StringBuffer qSvcName;
                            thebinding->qualifySubServiceName(*ctx,serviceName,NULL, qSvcName, NULL);
                            url.append(qSvcName);
                        }
                        else
                            thebinding->getServiceName(url);
                        url.append('/');
                        const char* parms = m_request->queryParamStr();
                        if (parms && *parms)
                            url.append('?').append(parms);
                        m_response->redirect(*m_request.get(),url);
                    }
                    else
                        thebinding->onGet(m_request.get(), m_response.get());
                }
                else
                    unsupported();
            }
            else
            {
                if(!stricmp(method.str(), POST_METHOD))
                    onPost();
                else if(!stricmp(method.str(), GET_METHOD))
                    onGet();
                else
                    unsupported();
            }
            ctx->addTraceSummaryTimeStamp(LogMin, "handleHttp");
        }
    }
    catch(IEspHttpException* e)
    {
        m_response->sendException(e);
        e->Release();
        return 0;
    }
    catch (IException *e)
    {
        DBGLOG(e);
        e->Release();
        return 0;
    }
    catch (...)
    {
        StringBuffer content_type;
        __int64 len = m_request->getContentLength();
        DBGLOG("Unknown Exception - processing request");
        DBGLOG("METHOD: %s, PATH: %s, TYPE: %s, CONTENT-LENGTH: %" I64F "d", m_request->queryMethod(), m_request->queryPath(), m_request->getContentType(content_type).str(), len);
        if (len > 0)
            m_request->logMessage(LOGCONTENT, "HTTP request content received:\n");
        return 0;
    }

    return 0;
}

int CEspHttpServer::onGetApplicationFrame(CHttpRequest* request, CHttpResponse* response, IEspContext* ctx)
{
    time_t modtime = 0;

    IProperties *params = request->queryParameters();
    const char *inner=(params)?params->queryProp("inner") : NULL;
    StringBuffer ifmodifiedsince;
    request->getHeader("If-Modified-Since", ifmodifiedsince);

    if (inner&&*inner&&ifmodifiedsince.length())
    {
        response->setStatus(HTTP_STATUS_NOT_MODIFIED);
        response->send();
    }
    else
    {
        CEspBindingEntry* entry = m_apport->getDefaultBinding();
        if(entry)
        {
            EspHttpBinding *httpbind = dynamic_cast<EspHttpBinding *>(entry->queryBinding());
            if(httpbind)
            {
                const char *page = httpbind->getRootPage(ctx);
                if(page && *page)
                    return onGetFile(request, response, page);
            }
        }

        StringBuffer html;
        m_apport->getAppFrameHtml(modtime, inner, html, ctx);
        response->setContent(html.length(), html.str());
        response->setContentType("text/html; charset=UTF-8");
        response->setStatus(HTTP_STATUS_OK);

        const char *timestr=ctime(&modtime);
        response->addHeader("Last-Modified", timestr);
        response->send();
    }

    return 0;
}

int CEspHttpServer::onGetTitleBar(CHttpRequest* request, CHttpResponse* response)
{
    bool rawXml = request->queryParameters()->hasProp("rawxml_");
    StringBuffer m_headerHtml(m_apport->getTitleBarHtml(*request->queryContext(), rawXml));
    response->setContent(m_headerHtml.length(), m_headerHtml.str());
    response->setContentType(rawXml ? HTTP_TYPE_APPLICATION_XML_UTF8 : "text/html; charset=UTF-8");
    response->setStatus(HTTP_STATUS_OK);
    response->send();
    return 0;
}

int CEspHttpServer::onGetNavWindow(CHttpRequest* request, CHttpResponse* response)
{
    StringBuffer navContent;
    StringBuffer navContentType;
    m_apport->getNavBarContent(*request->queryContext(), navContent, navContentType, request->queryParameters()->hasProp("rawxml_"));
    response->setContent(navContent.length(), navContent.str());
    response->setContentType(navContentType.str());
    response->setStatus(HTTP_STATUS_OK);
    response->send();
    return 0;
}

int CEspHttpServer::onGetDynNavData(CHttpRequest* request, CHttpResponse* response)
{
    StringBuffer navContent;
    StringBuffer navContentType;
    bool         bVolatile;
    m_apport->getDynNavData(*request->queryContext(), request->queryParameters(), navContent, navContentType, bVolatile);
    if (bVolatile)
        response->addHeader("Cache-control",  "max-age=0");
    response->setContent(navContent.length(), navContent.str());
    response->setContentType(navContentType.str());
    response->setStatus(HTTP_STATUS_OK);
    response->send();
    return 0;
}

int CEspHttpServer::onGetNavEvent(CHttpRequest* request, CHttpResponse* response)
{
    m_apport->onGetNavEvent(*request->queryContext(), request, response);
    return 0;
}

int CEspHttpServer::onGetBuildSoapRequest(CHttpRequest* request, CHttpResponse* response)
{
    m_apport->onBuildSoapRequest(*request->queryContext(), request, response);
    return 0;
}

#ifdef _USE_OPENLDAP
int CEspHttpServer::onUpdatePasswordInput(CHttpRequest* request, CHttpResponse* response)
{
    StringBuffer html;
    m_apport->onUpdatePasswordInput(*request->queryContext(), html);
    response->setContent(html.length(), html.str());
    response->setContentType("text/html; charset=UTF-8");
    response->setStatus(HTTP_STATUS_OK);

    response->send();

    return 0;
}

int CEspHttpServer::onUpdatePassword(CHttpRequest* request, CHttpResponse* response)
{
    StringBuffer html;
    m_apport->onUpdatePassword(*request->queryContext(), request, html);
    response->setContent(html.length(), html.str());
    response->setContentType("text/html; charset=UTF-8");
    response->setStatus(HTTP_STATUS_OK);

    response->send();
    return 0;
}
#endif

int CEspHttpServer::onGetMainWindow(CHttpRequest* request, CHttpResponse* response)
{
    StringBuffer url("../?main");
    double ver = request->queryContext()->getClientVersion();
    if (ver>0)
        url.appendf("&ver_=%g", ver);
    response->redirect(*request, url);
    return 0;
}

inline void make_env_var(StringArray &env, StringBuffer &var, const char *name, const char *value)
{
    env.append(var.clear().append(name).append('=').append(value).str());
}

inline void make_env_var(StringArray &env, StringBuffer &var, const char *name, const StringBuffer &value)
{
    env.append(var.clear().append(name).append('=').append(value).str());
}

inline void make_env_var(StringArray &env, StringBuffer &var, const char *name, __int64 value)
{
    env.append(var.clear().append(name).append('=').append(value).str());
}

bool skipHeader(const char *name)
{
    if (!stricmp(name, "CONTENT_LENGTH"))
        return true;
    else if (!strcmp(name, "AUTHORIZATION"))
        return true;
    else if (!strcmp(name, "CONTENT_TYPE"))
        return true;
    return false;
}

static void httpGetFile(CHttpRequest* request, CHttpResponse* response, const char *urlpath, const char *filepath)
{
    StringBuffer mimetype, etag, lastModified;
    MemoryBuffer content;
    bool modified = true;
    request->getHeader("If-None-Match", etag);
    request->getHeader("If-Modified-Since", lastModified);

    if (httpContentFromFile(filepath, mimetype, content, modified, lastModified, etag))
    {
        response->CheckModifiedHTTPContent(modified, lastModified.str(), etag.str(), mimetype.str(), content);
    }
    else
    {
        DBGLOG("Get File %s: file not found", filepath);
        response->setStatus(HTTP_STATUS_NOT_FOUND);
    }
    response->send();
}

static void httpGetDirectory(CHttpRequest* request, CHttpResponse* response, const char *urlpath, const char *dirpath, bool top, const StringBuffer &tail)
{
    Owned<IPropertyTree> tree = createPTree("directory", ipt_none);
    tree->setProp("@path", urlpath);
    Owned<IDirectoryIterator> dir = createDirectoryIterator(dirpath, NULL);
    ForEach(*dir)
    {
        IPropertyTree *entry = tree->addPropTree(dir->isDir() ? "directory" : "file", createPTree(ipt_none));
        StringBuffer s;
        entry->setProp("name", dir->getName(s));
        if (!dir->isDir())
            entry->setPropInt64("size", dir->getFileSize());
        CDateTime cdt;
        dir->getModifiedTime(cdt);
        entry->setProp("modified", cdt.getString(s.clear(), false));
    }

    const char *fmt = request->queryParameters()->queryProp("format");
    StringBuffer out;
    StringBuffer contentType;
    if (!fmt || strieq(fmt,"html"))
    {
        contentType.set("text/html");
        out.append("<!DOCTYPE html><html><body>");
        if (!top)
            out.appendf("<a href='%s'>..</a><br/>", tail.length() ? "." : "..");

        Owned<IPropertyTreeIterator> it = tree->getElements("*");
        ForEach(*it)
        {
            IPropertyTree &e = it->query();
            const char *href=e.queryProp("name");
            if (tail.length())
                out.appendf("<a href='%s/%s'>%s</a><br/>", tail.str(), href, href);
            else
                out.appendf("<a href='%s'>%s</a><br/>", href, href);
        }
        out.append("</body></html>");
    }
    else if (strieq(fmt, "json"))
    {
        contentType.set("application/json");
        toJSON(tree, out);
    }
    else if (strieq(fmt, "xml"))
    {
        contentType.set("application/xml");
        toXML(tree, out);
    }
    response->setStatus(HTTP_STATUS_OK);
    response->setContentType(contentType);
    response->setContent(out);
    response->send();
}

static bool checkHttpPathStaysWithinBounds(const char *path)
{
    if (!path || !*path)
        return true;
    int depth = 0;
    StringArray nodes;
    nodes.appendList(path, "/");
    ForEachItemIn(i, nodes)
    {
        const char *node = nodes.item(i);
        if (!*node || streq(node, ".")) //empty or "." doesn't advance
            continue;
        if (!streq(node, ".."))
            depth++;
        else
        {
            depth--;
            if (depth<0)  //only really care that the relative http path doesn't position itself above its own root node
                return false;
        }
    }
    return true;
}

int CEspHttpServer::onGetFile(CHttpRequest* request, CHttpResponse* response, const char *urlpath)
{
        if (!request || !response || !urlpath)
            return -1;

        StringBuffer basedir(getCFD());
        basedir.append("files/");

        if (!checkHttpPathStaysWithinBounds(urlpath))
        {
            DBGLOG("Get File %s: attempted access outside of %s", urlpath, basedir.str());
            response->setStatus(HTTP_STATUS_NOT_FOUND);
            response->send();
            return 0;
        }

        StringBuffer ext;
        StringBuffer tail;
        splitFilename(urlpath, NULL, NULL, &tail, &ext);

        bool top = !urlpath || !*urlpath;
        StringBuffer httpPath;
        request->getPath(httpPath).str();
        if (httpPath.charAt(httpPath.length()-1)=='/')
            tail.clear();
        else if (top)
            tail.set("./files");

        StringBuffer fullpath;
        makeAbsolutePath(urlpath, basedir.str(), fullpath);
        if (!checkFileExists(fullpath) && !checkFileExists(fullpath.toUpperCase()) && !checkFileExists(fullpath.toLowerCase()))
        {
            DBGLOG("Get File %s: file not found", urlpath);
            response->setStatus(HTTP_STATUS_NOT_FOUND);
            response->send();
            return 0;
        }

        if (isDirectory(fullpath))
            httpGetDirectory(request, response, urlpath, fullpath, top, tail);
        else
            httpGetFile(request, response, urlpath, fullpath);
        return 0;
}

int CEspHttpServer::onGetXslt(CHttpRequest* request, CHttpResponse* response, const char *path)
{
        if (!request || !response || !path)
            return -1;
        
        StringBuffer mimetype, etag, lastModified;
        MemoryBuffer content;
        bool modified = true;
        request->getHeader("If-None-Match", etag);
        request->getHeader("If-Modified-Since", lastModified);

        VStringBuffer filepath("%ssmc_xslt/%s", getCFD(), path);
        if (httpContentFromFile(filepath.str(), mimetype, content, modified, lastModified.clear(), etag) ||
            httpContentFromFile(filepath.clear().append(getCFD()).append("xslt/").append(path).str(), mimetype, content, modified, lastModified.clear(), etag))
        {
            response->CheckModifiedHTTPContent(modified, lastModified.str(), etag.str(), mimetype.str(), content);
        }
        else
        {
            DBGLOG("Get XSLT %s: file not found", filepath.str());
            response->setStatus(HTTP_STATUS_NOT_FOUND);
        }

        response->send();
        return 0;
}


int CEspHttpServer::unsupported()
{
    HtmlPage page("Enterprise Services Platform");
    StringBuffer espHeader;

    espHeader.append("<table border=\"0\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#000000\" height=\"108\">");
    espHeader.append("<tr><td width=\"24%\" height=\"24\" bgcolor=\"#000000\"><img border=\"0\" src=\"esp/files_/logo.gif\" width=\"258\" height=\"108\" /></td></tr>");
    espHeader.append("<tr><td width=\"24%\" height=\"24\" bgcolor=\"#AA0000\"><p align=\"center\" /><b><font color=\"#FFFFFF\" size=\"5\">Enterprise Services Platform</font></b></td></tr>");
    espHeader.append("</table>");

    page.appendContent(new CHtmlText(espHeader.str()));

    page.appendContent(new CHtmlHeader(H1, "Unsupported http method"));

    StringBuffer content;
    page.getHtml(content);

    m_response->setVersion(HTTP_VERSION);
    m_response->setContent(content.length(), content.str());
    m_response->setContentType("text/html; charset=UTF-8");
    m_response->setStatus(HTTP_STATUS_OK);

    m_response->send();

    return 0;
}

int CEspHttpServer::onOptions()
{
    m_response->setVersion(HTTP_VERSION);
    m_response->setStatus(HTTP_STATUS_OK);

    //CORS allow headers for interoperability, we do not rely on this for security since
    //that only means treating the browser as a trusted entity.  We need to be diligent and secure
    //for every request whether it comes from a cross domain browser or any other source
    StringBuffer allowHeaders;
    m_request->getHeader("Access-Control-Request-Headers", allowHeaders);
    if (allowHeaders.length())
        m_response->setHeader("Access-Control-Allow-Headers", allowHeaders);
    m_response->setHeader("Access-Control-Allow-Origin", "*");
    m_response->setHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    m_response->setHeader("Access-Control-Max-Age", "86400"); //arbitrary 24 hours
    m_response->setContentType("text/plain");
    m_response->setContent("");

    m_response->send();

    return 0;
}

int CEspHttpServer::onPost()
{
    HtmlPage page("Enterprise Services Platform");
    StringBuffer espHeader;

    espHeader.append("<table border=\"0\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#000000\" height=\"108\">");
    espHeader.append("<tr><td width=\"24%\" height=\"24\" bgcolor=\"#000000\"><img border=\"0\" src=\"esp/files_/logo.gif\" width=\"258\" height=\"108\" /></td></tr>");
    espHeader.append("<tr><td width=\"24%\" height=\"24\" bgcolor=\"#AA0000\"><p align=\"center\" /><b><font color=\"#FFFFFF\" size=\"5\">Enterprise Services Platform</font></b></td></tr>");
    espHeader.append("</table>");

    page.appendContent(new CHtmlText(espHeader.str()));

    page.appendContent(new CHtmlHeader(H1, "Invalid POST"));

    StringBuffer content;
    page.getHtml(content);

    m_response->setVersion(HTTP_VERSION);
    m_response->setContent(content.length(), content.str());
    m_response->setContentType("text/html; charset=UTF-8");
    m_response->setStatus(HTTP_STATUS_OK);

    m_response->send();

    return 0;
}

int CEspHttpServer::onGet()
{   
    if (m_request && m_request->queryParameters()->hasProp("config_") && m_viewConfig)
    {
        StringBuffer mimetype, etag, lastModified;
        MemoryBuffer content;
        bool modified = true;
        m_request->getHeader("If-None-Match", etag);
        m_request->getHeader("If-Modified-Since", lastModified);
        httpContentFromFile("esp.xml", mimetype, content, modified, lastModified, etag);
        m_response->setVersion(HTTP_VERSION);
        m_response->CheckModifiedHTTPContent(modified, lastModified.str(), etag.str(), HTTP_TYPE_APPLICATION_XML_UTF8, content);
        m_response->send();
    }
    else
    {
        HtmlPage page("Enterprise Services Platform");
        page.appendContent(new CHtmlHeader(H1, "Available Services:"));

        CHtmlList * list = (CHtmlList *)page.appendContent(new CHtmlList);
        EspHttpBinding* lbind=NULL;
        int ordinality=m_apport->getBindingCount();

        double ver = m_request->queryContext()->getClientVersion();
        for(int index=0; index<ordinality; index++)
        {
            CEspBindingEntry *entry = m_apport->queryBindingItem(index);
            lbind = (entry) ? dynamic_cast<EspHttpBinding*>(entry->queryBinding()) : NULL;
            if (lbind)
            {
                StringBuffer srv, srvLink;
                lbind->getServiceName(srv);
                srvLink.appendf("/%s", srv.str());
                if (ver)
                    srvLink.appendf("?ver_=%g", ver);
                
                list->appendContent(new CHtmlLink(srv.str(), srvLink.str()));
            }
        }

        StringBuffer content;
        page.getHtml(content);

        m_response->setVersion(HTTP_VERSION);
        m_response->setContent(content.length(), content.str());
        m_response->setContentType("text/html; charset=UTF-8");
        m_response->setStatus(HTTP_STATUS_OK);

        m_response->send();
    }

    return 0;
}

#ifdef _USE_OPENLDAP

EspAuthState CEspHttpServer::checkUserAuth()
{
    EspAuthRequest authReq;
    readAuthRequest(authReq);
    if(authReq.httpPath.isEmpty())
        throw MakeStringException(-1, "URL query string cannot be empty.");

    if (!authReq.authBinding)
        throw MakeStringException(-1, "Cannot find ESP HTTP Binding");

    DBGLOG("checkUserAuth: %s %s", m_request->isSoapMessage() ? "SOAP" : "HTTP", authReq.httpMethod.isEmpty() ? "??" : authReq.httpMethod.str());
    EspAuthState authState = preCheckAuth(authReq);
    if (authState != authUnknown)
        return authState;

    StringBuffer servName(authReq.ctx->queryServiceName(nullptr));
    if (servName.isEmpty())
    {
        authReq.authBinding->getServiceName(servName);
        authReq.ctx->setServiceName(servName.str());
    }

    AuthType domainAuthType = authReq.authBinding->getDomainAuthType();
    authReq.ctx->setDomainAuthType(domainAuthType);
    if (domainAuthType != AuthPerRequestOnly)
    {
        EspAuthState authState = checkUserAuthPerSession(authReq);
        if (authState != authUnknown)
            return authState;
    }
    if (domainAuthType != AuthPerSessionOnly)
    {// BasicAuthentication
        EspAuthState authState = checkUserAuthPerRequest(authReq);
        if (authState != authUnknown)
            return authState;
    }

    //authentication failed. Send out a login page or 401.
    StringBuffer userName;
    bool authSession =  false;
    if ((domainAuthType == AuthPerSessionOnly) || ((domainAuthType == AuthTypeMixed)
        && !authReq.ctx->getUserID(userName).length() && strieq(authReq.httpMethod.str(), GET_METHOD)))
    { //If session based, the first request comes from a browser using GET with no userID.
        authSession = true;
    }
    handleAuthFailed(authSession, authReq, nullptr);
    return authFailed;
}

void CEspHttpServer::readAuthRequest(EspAuthRequest& req)
{
    StringBuffer pathEx;
    m_request->getEspPathInfo(req.stype, &pathEx, &req.serviceName, &req.methodName, false);
    m_request->getMethod(req.httpMethod);
    m_request->getPath(req.httpPath);//m_httpPath

    req.isSoapPost = (strieq(req.httpMethod.str(), POST_METHOD) && m_request->isSoapMessage());
    req.ctx = m_request->queryContext();
    req.authBinding = getEspHttpBinding(req);
    req.requestParams = m_request->queryParameters();
}

EspAuthState CEspHttpServer::preCheckAuth(EspAuthRequest& authReq)
{
    if (!isAuthRequiredForBinding(authReq))
        return authSucceeded;

    if (!m_apport->rootAuthRequired() && strieq(authReq.httpMethod.str(), GET_METHOD) &&
        ((authReq.stype == sub_serv_root) || (!authReq.serviceName.isEmpty() && strieq(authReq.serviceName.str(), "esp"))))
        return authSucceeded;

    if (!authReq.httpMethod.isEmpty() && !authReq.serviceName.isEmpty() && !authReq.methodName.isEmpty() && strieq(authReq.serviceName.str(), "esp"))
    {
        if (strieq(authReq.httpMethod.str(), POST_METHOD) && strieq(authReq.methodName.str(), "updatepassword"))
        {
            EspHttpBinding* thebinding = getBinding();
            if (thebinding)
                thebinding->populateRequest(m_request.get());
            onUpdatePassword(m_request.get(), m_response.get());
            return authUpdatePassword;
        }
        if (strieq(authReq.httpMethod.str(), GET_METHOD) && strieq(authReq.methodName.str(), "updatepasswordinput"))//process before authentication check
        {
            onUpdatePasswordInput(m_request.get(), m_response.get());
            return authUpdatePassword;
        }
    }

    if((authReq.authBinding->getDomainAuthType() != AuthPerRequestOnly) && authReq.authBinding->isDomainAuthResources(authReq.httpPath.str()))
        return authSucceeded;//Give the permission to send out some pages used for logon or logout.

    return authUnknown;
}

EspAuthState CEspHttpServer::checkUserAuthPerSession(EspAuthRequest& authReq)
{
    ESPLOG(LogMax, "checkUserAuthPerSession");

    unsigned sessionID = readCookie(SESSION_ID_COOKIE);
    if (strieq(authReq.httpPath.str(), authReq.authBinding->getLogonURL()))
    {//Asking for logon page
        if (sessionID != 0)
            return authSucceeded;//If there is a valid session ID, give the permission to send out the logon page.

        ESPLOG(LogMin, "Authentication failed: no session ID found for logon page.");
        askUserLogOn(authReq.authBinding, createHTTPSession(authReq, nullptr, "/"));
        return authSucceeded;//Create a new session and redirect to the logon page.
    }

    if (sessionID > 0)
        return doSessionAuth(sessionID, authReq, nullptr, nullptr);

    if(!readCookie(SESSION_AUTH_COOKIE))
        return authUnknown;

    //session expired. But, userName/password exists. Should it be allowd?
    const char* userName = (authReq.requestParams) ? authReq.requestParams->queryProp("username") : NULL;
    const char* password = (authReq.requestParams) ? authReq.requestParams->queryProp("password") : NULL;
    if (!isEmptyString(userName) && !isEmptyString(password)) //from logon page.
        return doSessionAuth(createHTTPSession(authReq, nullptr, "/"), authReq, userName, password);

    if(authReq.isSoapPost) //from SOAP Test page
        sendMessage("Session expired. Please close this page and login again.", "text/html; charset=UTF-8");
    else //from other page
        askUserLogOn(authReq.authBinding, createHTTPSession(authReq, nullptr, nullptr));
    return authFailed;
}

EspAuthState CEspHttpServer::checkUserAuthPerRequest(EspAuthRequest& authReq)
{
    ESPLOG(LogMax, "checkUserAuthPerRequest");

    authReq.authBinding->populateRequest(m_request.get());
    if (authReq.authBinding->doAuth(authReq.ctx))
    {
        // authenticate optional groups. Do we still need?
        authOptionalGroups(authReq);
        StringBuffer userName, peer;
        ESPLOG(LogNormal, "Authenticated for %s@%s", authReq.ctx->getUserID(userName).str(), m_request->getPeer(peer).str());
        return authSucceeded;
    }
    if(!authReq.isSoapPost)
        return authUnknown;

    //If SoapPost, username/password may be in soap:Header which is not in HTTP header.
    //doAuth() will check them inside CSoapService::processHeader().
    authReq.ctx->setToBeAuthenticated(true);
    return authPending;
}

EspHttpBinding* CEspHttpServer::getEspHttpBinding(EspAuthRequest& authReq)
{
    if (strieq(authReq.httpMethod.str(), GET_METHOD) && ((authReq.stype == sub_serv_root)
            || (!authReq.serviceName.isEmpty() && strieq(authReq.serviceName.str(), "esp"))))
        return getBinding();

    if(!m_apport)
        return nullptr;

    int ordinality=m_apport->getBindingCount();
    if (ordinality < 1)
        return nullptr;

    EspHttpBinding* espHttpBinding = nullptr;
    if (ordinality==1)
    {
        CEspBindingEntry *entry = m_apport->queryBindingItem(0);
        espHttpBinding = (entry) ? dynamic_cast<EspHttpBinding*>(entry->queryBinding()) : NULL;
        //Not sure why check !isSoapPost if (ordinality==1)
        //The isValidServiceName() returns false except for ws_ecl and esdl.
        if (!authReq.isSoapPost && espHttpBinding && !espHttpBinding->isValidServiceName(*authReq.ctx, authReq.serviceName.str()))
            espHttpBinding=nullptr;
        return espHttpBinding;
    }

    for(unsigned index=0; index<ordinality; index++)
    {
        CEspBindingEntry *entry = m_apport->queryBindingItem(index);
        EspHttpBinding* lbind = (entry) ? dynamic_cast<EspHttpBinding*>(entry->queryBinding()) : nullptr;
        if (lbind && lbind->isValidServiceName(*authReq.ctx, authReq.serviceName.str()))
        {
            espHttpBinding=lbind;
            break;
        }
    }

    if (!espHttpBinding && m_defaultBinding)
        espHttpBinding=dynamic_cast<EspHttpBinding*>(m_defaultBinding.get());

    return espHttpBinding;
}

bool CEspHttpServer::isAuthRequiredForBinding(EspAuthRequest& authReq)
{
    IAuthMap* authmap = authReq.authBinding->queryAuthMAP();
    if(!authmap) //No auth requirement
        return false;

    const char* authMethod = authReq.authBinding->queryAuthMethod();
    if (isEmptyString(authMethod) || strieq(authMethod, "none"))
        return false;

    ISecResourceList* rlist = authmap->getResourceList(authReq.httpPath.str());
    if(!rlist) //No auth requirement for the httpPath.
        return false;

    authReq.ctx->setAuthenticationMethod(authMethod);
    authReq.ctx->setResources(rlist);

    return true;
}

void CEspHttpServer::sendMessage(const char* msg, const char* msgType)
{
    m_response->setContent(msg);
    m_response->setContentType(msgType);
    m_response->setStatus(HTTP_STATUS_OK);
    m_response->send();
}

EspAuthState CEspHttpServer::doSessionAuth(unsigned sessionID, EspAuthRequest& authReq, const char* _userName, const char* _password)
{
    ESPLOG(LogMax, "doSessionAuth: %s<%d>", PropSessionID, sessionID);

    CDateTime now;
    now.setNow();
    time_t accessTime = now.getSimple();
    Owned<IRemoteConnection> conn = querySDS().connect(authReq.authBinding->getDomainSessionSDSPath(), myProcessSession(), RTM_LOCK_WRITE, SESSION_SDS_LOCK_TIMEOUT);
    if (!conn)
        throw MakeStringException(-1, "Failed to connect SDS DomainSession.");

    IPropertyTree* domainSessions = readAndCleanDomainSessions(authReq.authBinding, conn, accessTime);
    VStringBuffer xpath("%s[%s='%d']", PathSessionSession, PropSessionID, sessionID);
    IPropertyTree* sessionTree = domainSessions->getBranch(xpath.str());
    if (!sessionTree)
    {
        ESPLOG(LogMin, "Authentication failed: session:<%d> not found", sessionID);
        if(authReq.isSoapPost) //from SOAP Test page
            sendMessage("Session expired. Please close this page and login again.", "text/html; charset=UTF-8");
        else
            askUserLogOn(authReq.authBinding, createHTTPSession(authReq, domainSessions, nullptr));

        commitAndCloseRemoteConnection(conn);
        return authFailed;
    }

    HTTPSessionState sessionState = (HTTPSessionState) sessionTree->getPropInt(PropSessionState);
    if (sessionState == HTTPSS_new)
    {
        StringBuffer userName, password;
        if (!isEmptyString(_userName))
            userName.set(_userName);
        else
            userName.set((authReq.requestParams) ? authReq.requestParams->queryProp("username") : nullptr);
        if (!isEmptyString(_password))
            password.set(_password);
        else
            password.set((authReq.requestParams) ? authReq.requestParams->queryProp("password") : nullptr);
        if (userName.isEmpty() || password.isEmpty())
        {
            ESPLOG(LogMin, "Authentication failed: invalid credential for session:<%d>", sessionID);
            handleAuthFailed(true, authReq, &sessionID);
            commitAndCloseRemoteConnection(conn);
            return authFailed;
        }

        authReq.ctx->setUserID(userName.str());
        authReq.ctx->setPassword(password.str());
        authReq.authBinding->populateRequest(m_request.get());
        if (!authReq.authBinding->doAuth(authReq.ctx))
        {
            ESPLOG(LogMin, "Authentication failed for session:<%d>", sessionID);
            handleAuthFailed(true, authReq, &sessionID);
            commitAndCloseRemoteConnection(conn);
            return authFailed;
        }

        authReq.ctx->addUserToken(sessionID);

        // authenticate optional groups
        authOptionalGroups(authReq);

        sessionTree->setProp(PropSessionUserID, userName.str());
        sessionTree->addPropInt(PropSessionState, HTTPSS_active);
    }
    else
    {
        authOptionalGroups(authReq);

        authReq.ctx->setUserID(sessionTree->queryProp(PropSessionUserID));
        authReq.authBinding->populateRequest(m_request.get());
        authReq.ctx->addUserToken(sessionID);
        authReq.ctx->queryUser()->setPropertyInt("ESPSessionID", sessionID);
    }

    postSessionAuth(authReq, sessionID, sessionState, accessTime, conn, sessionTree);

    return authSucceeded;
}

void CEspHttpServer::postSessionAuth(EspAuthRequest& authReq, unsigned sessionID, HTTPSessionState sessionState, time_t accessTime,
    void* _conn, IPropertyTree* sessionTree)
{
    if (!_conn)
        throw MakeStringException(-1, "Invalid SDS connection.");

    StringBuffer userName = sessionTree->queryProp(PropSessionUserID);
    StringBuffer netAddr = sessionTree->queryProp(PropSessionNetworkAddress);
    sessionTree->setPropInt64(PropSessionLastAccessed, accessTime);
    IRemoteConnection* conn = (IRemoteConnection*) _conn;
    if (authReq.methodName && strieq(authReq.methodName, "logout"))
    {//delete this session before logout
        IPropertyTree* root = conn->queryRoot();
        if (root)
        {
            VStringBuffer path("%s[%s='%d']", PathSessionSession, PropSessionID, sessionID);
            Owned<IPropertyTreeIterator> it = root->getElements(path.str());
            ForEach(*it)
                root->removeTree(&it->query());
        }
    }
    commitAndCloseRemoteConnection(conn);

    ///authReq.ctx->setAuthorized(true);
    VStringBuffer sessionIDStr("%d", sessionID);
    addCookie(SESSION_ID_COOKIE, sessionIDStr.str(), authReq.authBinding->getSessionTimeoutSeconds());
    ESPLOG(LogMax, "Authenticated for %s@%s", userName.str(), netAddr.str());

    if (sessionState == HTTPSS_new)
    {
        const char* redirectURL = sessionTree->queryProp(PropSessionLoginURL);
        if (!isEmptyString(redirectURL))
        {
            const char* logonURL = authReq.authBinding->getLogonURL();
            if (!strieq(logonURL, redirectURL))
                m_response->redirect(*m_request, redirectURL);
            else
                m_response->redirect(*m_request, "/");
        }
        else
            m_response->redirect(*m_request, "/");
    }
    else if (authReq.methodName && strieq(authReq.methodName, "logout"))
    {
        clearCookie(SESSION_ID_COOKIE);
        const char* logoutURL = authReq.authBinding->getLogoutURL();
        if (!isEmptyString(logoutURL))
            m_response->redirect(*m_request, authReq.authBinding->getLogoutURL());
        else
            send200OK();
    }
}

void CEspHttpServer::handleAuthFailed(bool sessionAuth, EspAuthRequest& authReq, unsigned* sessionID)
{
    ISecUser *user = authReq.ctx->queryUser();
    if (user && (user->getAuthenticateStatus() == AS_PASSWORD_EXPIRED || user->getAuthenticateStatus() == AS_PASSWORD_VALID_BUT_EXPIRED))
    {
        ESPLOG(LogMin, "ESP password expired for %s", authReq.ctx->queryUserId());
        handlePasswordExpired(sessionAuth);
        return;
    }

    if (!sessionAuth)
    {
        ESPLOG(LogMin, "Authentication failed: send BasicAuthentication.");
        m_response->sendBasicChallenge(authReq.authBinding->getChallengeRealm(), true);
        return;
    }

    ESPLOG(LogMin, "Authentication failed: askUserLogOn.");
    if (sessionID)
        askUserLogOn(authReq.authBinding, *sessionID);
    else
        askUserLogOn(authReq.authBinding, createHTTPSession(authReq, nullptr, nullptr));
}

void CEspHttpServer::askUserLogOn(EspHttpBinding* authBinding, unsigned sessionID)
{
    VStringBuffer sessionIDStr("%d", sessionID);
    addCookie(SESSION_ID_COOKIE, sessionIDStr.str(), authBinding->getSessionTimeoutSeconds());
    addCookie(SESSION_AUTH_COOKIE, "1", 0); //time out when browser is closed
    m_response->redirect(*m_request, authBinding->getLogonURL());
}

void CEspHttpServer::send200OK()
{
    m_response->setContentType("text/html; charset=UTF-8");
    m_response->setStatus(HTTP_STATUS_OK);
    m_response->send();
}

unsigned CEspHttpServer::createHTTPSession(EspAuthRequest& authReq, IPropertyTree* domainSessions, const char* _loginURL)
{
    CDateTime now;
    now.setNow();
    time_t createTime = now.getSimple();

    StringBuffer peer;
    VStringBuffer idStr("%s_%ld", m_request->getPeer(peer).str(), createTime);
    unsigned sessionID = hashc((unsigned char *)idStr.str(), idStr.length(), 0);
    ESPLOG(LogMax, "New sessionID <%d> at <%ld> in createHTTPSession()", sessionID, createTime);

    Owned<IPropertyTree> ptree = createPTree();
    ptree->addProp(PropSessionNetworkAddress, peer.str());
    ptree->addPropInt(PropSessionID, sessionID);
    ptree->addPropInt(PropSessionState, HTTPSS_new);
    ptree->setPropInt64(PropSessionCreateTime, createTime);
    ptree->setPropInt64(PropSessionLastAccessed, createTime);

    if (!isEmptyString(_loginURL))
        ptree->addProp(PropSessionLoginURL, _loginURL);
    else
    {
        StringBuffer loginURL = authReq.httpPath;
        if (authReq.requestParams && authReq.requestParams->hasProp("__querystring"))
            loginURL.append("?").append(authReq.requestParams->queryProp("__querystring"));
        if (!loginURL.isEmpty() && streq(loginURL.str(), "/WsSMC/"))
            ptree->addProp(PropSessionLoginURL, "/");
        else
            ptree->addProp(PropSessionLoginURL, loginURL.str());
    }

    if (domainSessions)
    {
        domainSessions->addPropTree(PathSessionSession, LINK(ptree));
        return sessionID;
    }

    Owned<IRemoteConnection> conn = querySDS().connect(authReq.authBinding->getDomainSessionSDSPath(), myProcessSession(), RTM_LOCK_WRITE, SESSION_SDS_LOCK_TIMEOUT);
    if (!conn)
        throw MakeStringException(-1, "Failed to connect SDS DomainSession.");

    IPropertyTree* root = conn->queryRoot();
    if (!root)
        throw MakeStringException(-1, "Failed to get SDS DomainSession.");

    root->addPropTree(PathSessionSession, LINK(ptree));
    return sessionID;
}

IPropertyTree* CEspHttpServer::readAndCleanDomainSessions(EspHttpBinding* authBinding, void* _conn, time_t accessTime)
{
    if (!_conn)
        throw MakeStringException(-1, "Invalid SDS connection.");

    IRemoteConnection* conn = (IRemoteConnection*) _conn;
    IPropertyTree* root = conn->queryRoot();
    if (!root)
        throw MakeStringException(-1, "Failed to get SDS DomainSession.");

    //Removing HTTPSessions if timed out
    Owned<IPropertyTreeIterator> iter = root->getElements(PathSessionSession);
    ForEach(*iter)
    {
        IPropertyTree& item = iter->query();
        if (accessTime - item.getPropInt64(PropSessionLastAccessed, 0) >= authBinding->getSessionTimeoutSeconds())
            root->removeTree(&item);
    }
    return root;
}

bool CEspHttpServer::commitAndCloseRemoteConnection(void* _conn)
{
    if (!_conn)
        return false;

    IRemoteConnection* conn = (IRemoteConnection*) _conn;
    conn->commit();
    conn->close();
    return true;
}

void CEspHttpServer::handlePasswordExpired(bool sessionAuth)
{
    if (sessionAuth)
        m_response->redirect(*m_request.get(), "/esp/updatepasswordinput");
    else
    {
        Owned<IMultiException> me = MakeMultiException();
        me->append(*MakeStringException(-1, "Your ESP password has expired."));
        m_response->handleExceptions(nullptr, me, "ESP Authentication", "PasswordExpired", nullptr);
    }
    return;
}

void CEspHttpServer::authOptionalGroups(EspAuthRequest& authReq)
{
    if (strieq(authReq.httpMethod.str(), GET_METHOD) && (authReq.stype==sub_serv_root) && authenticateOptionalFailed(*authReq.ctx, nullptr))
        throw MakeStringException(-1, "Unauthorized Access");
    if ((!strieq(authReq.httpMethod.str(), GET_METHOD) || !strieq(authReq.serviceName.str(), "esp")) && authenticateOptionalFailed(*authReq.ctx, authReq.authBinding))
        throw MakeStringException(-1, "Unauthorized Access");
}

void CEspHttpServer::addCookie(const char* cookieName, const char *cookieValue, unsigned maxAgeSec)
{
    CEspCookie* cookie = new CEspCookie(cookieName, cookieValue);
    if (maxAgeSec > 0)
    {
        char expiresTime[64];
        time_t tExpires;
        time(&tExpires);
        tExpires += maxAgeSec;
#ifdef _WIN32
        struct tm *gmtExpires;
        gmtExpires = gmtime(&tExpires);
        strftime(expiresTime, 64, "%a, %d %b %Y %H:%M:%S GMT", gmtExpires);
#else
        struct tm gmtExpires;
        gmtime_r(&tExpires, &gmtExpires);
        strftime(expiresTime, 64, "%a, %d %b %Y %H:%M:%S GMT", &gmtExpires);
#endif //_WIN32

        cookie->setExpires(expiresTime);
    }
    cookie->setHTTPOnly(true);
    m_response->addCookie(cookie);
}

void CEspHttpServer::clearCookie(const char* cookieName)
{
    CEspCookie* cookie = new CEspCookie(cookieName, "");
    cookie->setExpires("Thu, 01 Jan 1970 00:00:01 GMT");
    m_response->addCookie(cookie);
    m_response->addHeader(cookieName,  "max-age=0");
}

unsigned CEspHttpServer::readCookie(const char* cookieName)
{
    CEspCookie* sessionIDCookie = m_request->queryCookie(cookieName);
    if (sessionIDCookie)
    {
        StringBuffer sessionIDStr = sessionIDCookie->getValue();
        if (sessionIDStr.length())
            return atoi(sessionIDStr.str());
    }
    return 0;
}

#endif
