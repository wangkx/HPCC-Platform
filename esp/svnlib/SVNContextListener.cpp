#include <iostream>



// svncpp
#include "m_check.hpp"
#include "SVNContextListener.hpp"

#include "svncpp/context.hpp"


void
SVNContextListener::contextNotify (const char *path,
                         svn_wc_notify_action_t action,
                         svn_node_kind_t kind,
                         const char *mime_type,
                         svn_wc_notify_state_t content_state,
                         svn_wc_notify_state_t prop_state,
                         svn_revnum_t revision)
{
  static const char *
  ACTION_NAMES [] =
  {
    ("Add"),              // svn_wc_notify_add,
    ("Copy"),             // svn_wc_notify_copy,
    ("Delete"),           // svn_wc_notify_delete,
    ("Restore"),          // svn_wc_notify_restore,
    ("Revert"),           // svn_wc_notify_revert,
    NULL ,                 // NOT USED HERE svn_wc_notify_failed_revert,
    ("Resolved"),         // svn_wc_notify_resolved,
    ("Skip"),             // NOT USED HERE svn_wc_notify_skip,
    ("Deleted"),          // svn_wc_notify_update_delete,
    ("Added"),            // svn_wc_notify_update_add,
    ("Updated"),          // svn_wc_notify_update_update,
    NULL,                  // NOT USED HERE svn_wc_notify_update_completed,
    NULL,                  // NOT USED HERE svn_wc_notify_update_external,
    NULL,                  // NOT USED HERE svn_wc_notify_status_completed
    NULL,                  // NOT USED HERE svn_wc_notify_status_external
    ("Modified"),         // svn_wc_notify_commit_modified,
    ("Added"),            // svn_wc_notify_commit_added,
    ("Deleted"),          // svn_wc_notify_commit_deleted,
    ("Replaced"),         // svn_wc_notify_commit_replaced,
    NULL,                  // NOT USED HERE svn_wc_notify_commit_postfix_txdelta
    NULL,                  // NOT USED HERE svn_wc_notify_blame_revision
    ("Locked"),           // svn_wc_notify_locked
    ("Unlocked"),         // svn_wc_notify_unlocked
    ("Failed to lock"),   // svn_wc_notify_failed_lock
    ("Failed to unlock")  // svn_wc_notify_failed_unlock
  };

  static const int MAX_ACTION = svn_wc_notify_failed_unlock;

  if (ACTION_NAMES[action] != NULL && action >= 0 && action <= MAX_ACTION)
  {
     //just print out the event...
      std::cout << "ContextNotify: " << ACTION_NAMES[action] << ": " << path << std::endl;
  }
}

bool
SVNContextListener::contextGetLogin (const std::string & realm,
                           std::string & username,
                           std::string & password,
                           bool & maySave)
{

  std::cout << "Inside SVNContextListener::contextGetLogin()..." <<std::cout;
  bool success = true;
  return success;
}

bool
SVNContextListener::contextGetLogMessage (std::string & msg)
{
    
  std::cout << "Inside SVNContextListener::contextGetLogMessage()..." << std::cout;

  msg = m_context->getLogMessage(); // m->message;
  
  return true;
}


svn::ContextListener::SslServerTrustAnswer

SVNContextListener::contextSslServerTrustPrompt (
  const svn::ContextListener::SslServerTrustData & data,
  apr_uint32_t & acceptedFailures)
{
  
  std::cout << "Inside SVNContextListener::contextSslServerTrustPrompt()..." << std::cout;

  m_sslServerTrustData = data;

  //callbackSslServerTrustPrompt ();

  return m_sslServerTrustAnswer;
}


bool
SVNContextListener::contextSslClientCertPrompt (std::string & certFile)
{
  std::cout << "Inside SVNContextListener::contextSslClientCertPrompt()..." << std::cout;

  /* 
  certFile = m->certFile;
  */
  return false;
}


bool
SVNContextListener::contextSslClientCertPwPrompt (std::string & password,
                                        const std::string & realm,
                                        bool & maySave)
{
  std::cout << "Inside SVNContextListener::contextSslClientCertPwPrompt()..." << std::cout;

  /* 
  /// @todo @ref realm isnt used yet
  /// @todo @ref maySave isnt used yet
  m->password = password;
  
  m->sendSignalAndWait (SIG_SSL_CLIENT_CERT_PW_PROMPT);

  bool success = m->dataReceived;
  m->dataReceived = false;

  if(!success)
    return false;

  password = m->password;
  */ 

  return false;
}

bool
SVNContextListener::contextCancel ()
{
  return m_isCancelled;
}

bool
SVNContextListener::isCancelled () const
{
  return m_isCancelled;
}

void
SVNContextListener::cancel (bool value)
{
  m_isCancelled = value;
}



/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rapidsvn-dev.el")
 * end:
 */
