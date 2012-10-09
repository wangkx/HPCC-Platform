
#ifndef _LISTENER_H
#define _LISTENER_H

// svncpp
#include "svncpp/context_listener.hpp"
#include "svncpp/path.hpp"
#include "svncpp/targets.hpp"


namespace svn
{
  class Context;
}

/**
 * Class that listens to the @a svn::ContextListener interface
 * and forwards information to the app and requests informations
 * like authentication and such
 */
class SVNContextListener : public svn::ContextListener
{
public:
  /**
   * constructor
   *
   * @param parent parent window
   */
    SVNContextListener (svn::Context * context) 
    : 
    m_context( context ), m_isCancelled(false)
    {}

  /**
   * destructor
   */
    virtual ~SVNContextListener () {}

  /**
   * sets the context
   *
   * @param context
   */
  void SetContext (svn::Context * context)
  {
      m_context = context;
  }

  /**
   * @return the context of the action
   */
  svn::Context * GetContext ()
  {
      return m_context;
  }

  /**
   * @see svn::ContextListener
   */
  virtual bool
  contextGetLogin (const std::string & realm,
                   std::string & username,
                   std::string & password,
                   bool & maySave);

  /**
   * @see svn::ContextListener
   */
  virtual void
  contextNotify (const char *path,
                 svn_wc_notify_action_t action,
                 svn_node_kind_t kind,
                 const char *mime_type,
                 svn_wc_notify_state_t content_state,
                 svn_wc_notify_state_t prop_state,
                 svn_revnum_t revision);

  /**
   * @see svn::ContextListener
   */
  virtual bool
  contextGetLogMessage (std::string & msg);

  /**
   * @see svn::ContextListener
   */
  virtual svn::ContextListener::SslServerTrustAnswer
  contextSslServerTrustPrompt (
    const svn::ContextListener::SslServerTrustData & data,
    apr_uint32_t & acceptedFailures);

  /**
   * @see svn::ContextListener
   */
  virtual bool
  contextSslClientCertPrompt (std::string & certFile);

  /**
   * @see svn::ContextListener
   */
  virtual bool
  contextSslClientCertPwPrompt (std::string & password,
                                const std::string & realm,
                                bool & maySave);

  /**
   * @see svn::ContextListener
   */
  virtual bool
  contextCancel ();

  /**
   * shall the ongoing operation be cancelled?
   *
   * @see isCancelled
   *
   * @param value
   *  @li true cancel operation
   *  @li false dont cancel
   */
  void
  cancel (bool value);

  /**
   * @see cancel
   * @return check whether the ongoing operation is to be cancelled
   * @retval true cancel
   * @retval false dont cancel
   */
  bool
  isCancelled () const;


protected:


private:
  /**
   * the context under which the action will be
   * performed. this includes auth info and callback
   * addresses
   */
  svn::Context * m_context;
  svn::Targets m_targets;

  svn::ContextListener::SslServerTrustAnswer m_sslServerTrustAnswer;
  svn::ContextListener::SslServerTrustData   m_sslServerTrustData;
  apr_uint32_t m_acceptedFailures;

  /**
   * is ongoing operation to be cancelled?
   * will be evaluated by Listener::contextCancel
   */

  bool m_isCancelled;

  /**
   * Shared between two threads data
   */
  std::string message;
  std::string username;
  std::string password;
  std::string certFile;

  /**
   * disable private default constructor
   */
  SVNContextListener ();

  /**
   * disable private copy constructor
   */
  SVNContextListener (const SVNContextListener &);
};

#endif

