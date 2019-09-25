#ifndef LIM_BOOTSTRAP_H
#define LIM_BOOTSTRAP_H
#include <lim/config.h>
#include <lim/base/bootstrap_config.h>
#ifdef ENABLE_OPENSSL
#include <lim/base/sslcontext.h>
#endif

namespace lim {
  class Bootstrap {
  public:
    Bootstrap(BootstrapConfig &config): config_(config) {
#ifdef ENABLE_OPENSSL
      server_ssl_context_ = NULL;
			client_ssl_context_ = NULL;
#endif
    }

    virtual ~Bootstrap() = default;
		
#ifdef ENABLE_OPENSSL
    void SetServerSSLContext(SSLContext *server_ssl_context) { server_ssl_context_ = server_ssl_context; };
    SSLContext *GetServerSSLContext() { return server_ssl_context_; };

		void SetClientSSLContext(SSLContext *client_ssl_context) { client_ssl_context_ = client_ssl_context; };
    SSLContext *GetClientSSLContext() { return client_ssl_context_; };
#endif

    template<typename T>
		bool Bind(const std::string &unix_path, std::function<void(T&)> init_params_callback = NULL) {
      SocketChannel channel;
#ifdef ENABLE_OPENSSL
      if (server_ssl_context_ != NULL) {
        channel.SetSSLContext(server_ssl_context_);
      }
#endif
      if (!channel.Bind(unix_path)) {
        return false;
      }

      T *server = new T(channel, config_);
			if (init_params_callback != NULL) {
				init_params_callback(*server);
			}
      server->Signal(ExecuteEvent::INIT_EVENT);
      return true;
    }
		
    template<typename T>
		bool Bind(const std::string &local_ip, int local_port, std::function<void(T&)> init_params_callback = NULL) {
      SocketChannel channel;
#ifdef ENABLE_OPENSSL
      if (server_ssl_context_ != NULL) {
        channel.SetSSLContext(server_ssl_context_);
      }
#endif
      if (!channel.Bind(local_ip, local_port)) {
        return false;
      }

      T *server = new T(channel, config_);
			if (init_params_callback != NULL) {
				init_params_callback(*server);
			}
      server->Signal(ExecuteEvent::INIT_EVENT);
      return true;
    }

    template<typename T>
		bool Connect(const std::string &unix_path, std::function<void(T&)> init_params_callback = NULL) {
      SocketChannel channel;
#ifdef ENABLE_OPENSSL
      if (client_ssl_context_ != NULL) {
        channel.SetSSLContext(client_ssl_context_);
      }
#endif
      if (!channel.Connect(unix_path)) {
        return false;
      }

      T *client = new T(channel, config_);
			if (init_params_callback != NULL) {
				init_params_callback(*client);
			}
      client->Signal(ExecuteEvent::INIT_EVENT);
      return true;
    }

    template<typename T>
		bool Connect(const std::string &remote_ip, int remote_port, std::function<void(T&)> init_params_callback = NULL) {
      SocketChannel channel;
#ifdef ENABLE_OPENSSL
      if (client_ssl_context_ != NULL) {
        channel.SetSSLContext(client_ssl_context_);
      }
#endif
      if (!channel.Connect(remote_ip, remote_port)) {
        return false;
      }
			
      T *client = new T(channel, config_);
			if (init_params_callback != NULL) {
				init_params_callback(*client);
			}
      client->Signal(ExecuteEvent::INIT_EVENT);
      return true;
    }

  protected:
    BootstrapConfig &config_;
#ifdef ENABLE_OPENSSL
    SSLContext *server_ssl_context_;
		SSLContext *client_ssl_context_;
#endif
  };
}
#endif
