/*
 * Copyright (C) 2009-2011 S[&]T, The Netherlands.
 *
 * This file is part of the QIAP Toolkit.
 *
 * The QIAP Toolkit is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The QIAP Toolkit is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the QIAP Toolkit; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "qiap.h"

#define USER_AGENT "User-Agent: QIAP library v" QIAP_VERSION

int qiap_handle_soap_response(int fd, qiap_quality_issue_report **quality_issue_report);

static int connect_to_server(const char *hostname, unsigned short port)
{
    struct hostent *hostinfo;
    int socketID = -1;

    hostinfo = gethostbyname(hostname);
    if (hostinfo != NULL)
    {
        struct sockaddr_in ip_address;

        assert(hostinfo->h_addr_list[0] != NULL);

        socketID = socket(PF_INET, SOCK_STREAM, 0);
        if (socketID == -1)
        {
            qiap_set_error(QIAP_ERROR_SERVER, "could not open socket to connect to host %s:%u - %s", hostname, port,
                           strerror(errno));
            return -1;
        }

        ip_address.sin_addr = *(struct in_addr *)hostinfo->h_addr_list[0];
        ip_address.sin_family = AF_INET;
        ip_address.sin_port = htons(port);
        if (connect(socketID, (struct sockaddr *)&ip_address, sizeof(ip_address)) != 0)
        {
            qiap_set_error(QIAP_ERROR_SERVER, "could not connect to host %s:%u - %s", hostname, port, strerror(errno));
            close(socketID);
            return -1;
        }
    }
    else
    {
        qiap_set_error(QIAP_ERROR_SERVER, "could not resolve hostname '%s'", hostname);
        switch (h_errno)
        {
            case HOST_NOT_FOUND:
                qiap_add_error_message(" - host not found");
                break;
            case NO_DATA:
                qiap_add_error_message(" - no address available for this host");
                break;
            case NO_RECOVERY:
                qiap_add_error_message(" - name server error");
                break;
        }
        return -1;
    }

    return socketID;
}

void add_string_to_string(char **string, const char *additional_string)
{
    if (*string == NULL)
    {
        *string = strdup(additional_string);
        assert(*string != NULL);
    }
    else
    {
        char *new_string;

        new_string = realloc(*string, strlen(*string) + strlen(additional_string) + 1);
        assert(new_string != NULL);
        strcat(new_string, additional_string);
        *string = new_string;
    }
}

static char *create_soap_request(const char *username, const char *password, const qiap_query *query)
{
    char *request = NULL;
    int i;

    add_string_to_string(&request, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                         "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
                         "xmlns:xml=\"http://www.w3.org/XML/1998/namespace\">\n");
    if (username != NULL)
    {
        add_string_to_string(&request, "<soap:Header>\n<wsse:Security><wsse:UsernameToken><wsse:UserName>");
        add_string_to_string(&request, username);
        add_string_to_string(&request, "</wsse:UserName>");
        if (password != NULL)
        {
            add_string_to_string(&request, "<wsse:Password>");
            add_string_to_string(&request, username);
            add_string_to_string(&request, "</wsse:Password>");
        }
        add_string_to_string(&request, "</wsse:UsernameToken></wsse:Security>\n</soap:Header>\n");
    }
    else
    {
        add_string_to_string(&request, "<soap:Header/>\n");
    }
    add_string_to_string(&request, "<soap:Body>\n");
    add_string_to_string(&request, "<qq:QualityIssueQuery xmlns:qq=\"http://geca.esa.int/qiap/query/2008/07\">\n");
    for (i = 0; i < query->num_entries; i++)
    {
        add_string_to_string(&request, "<qq:ProductType mission=\"");
        add_string_to_string(&request, query->mission[i]);
        add_string_to_string(&request, "\">");
        add_string_to_string(&request, query->product_type[i]);
        add_string_to_string(&request, "</qq:ProductType>\n");
    }
    add_string_to_string(&request, "</qq:QualityIssueQuery>\n</soap:Body>\n</soap:Envelope>\n");

    return request;
}

int parse_serverurl(const char *serverurl, char **host, int *port, char **path)
{
    const char *p_port;
    const char *p_path;
    const char *p_host;
    const char *p_userinfo;
    const char *p;
    int length;

    p = strchr(serverurl, ':');
    if (p != NULL && p[1] == '/' && p[2] == '/')
    {
        if (memcmp(serverurl, "http:", 5) != 0 && memcmp(serverurl, "HTTP:", 5) != 0)
        {
            qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid server url (url scheme should be 'http')");
            return -1;
        }
        p_userinfo = &serverurl[7];
    }
    else
    {
        p_userinfo = serverurl;
    }

    p_path = strchr(p_userinfo, '/');
    p_host = strchr(p_userinfo, '@');
    if (p_host != NULL && (p_path == NULL || p_host < p_path))
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid server url (http user authentication not supported)");
        return -1;
    }
    else
    {
        p_host = p_userinfo;
        p_userinfo = NULL;
    }

    /* determine port number */
    p_port = strchr(p_host, ':');
    if (p_port != NULL && (p_path == NULL || p_port < p_path))
    {
        if (p_path == NULL)
        {
            length = strlen(p_port);
        }
        else
        {
            length = p_path - p_port;
        }
        *port = 0;
        for (p = &p_port[1]; p < &p_port[length]; p++)
        {
            if (*p < '0' || *p > '9')
            {
                qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid server url (invalid server port number)");
                return -1;
            }
            *port = 10 * (*port) + (*p - '0');
            if (*port > 65535)
            {
                qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid server url (invalid server port number)");
                return -1;
            }
        }
    }
    else
    {
        p_port = NULL;
        /* default http port */
        *port = 80;
    }

    /* determine hostname/ip-number */
    if (p_port != NULL)
    {
        length = p_port - p_host;
    }
    else if (p_path != NULL)
    {
        length = p_path - p_host;
    }
    else
    {
        length = strlen(p_host);
    }
    if (length == 0)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid server url (no server specified)");
        return -1;
    }
    *host = malloc(length + 1);
    assert(*host != NULL);
    strncpy(*host, p_host, length);
    (*host)[length] = '\0';

    /* determine path */
    if (p_path == NULL)
    {
        *path = strdup("/");
    }
    else
    {
        *path = strdup(p_path);
    }
    assert(*path != NULL);

    return 0;
}

LIBQIAP_API int qiap_query_server(const char *serverurl, const char *username, const char *password,
                                  const qiap_query *query, qiap_quality_issue_report **quality_issue_report)
{
    char buffer[1024];
    char *body;
    char *host;
    char *path;
    int port;
    int length;
    int fd;

    if (serverurl == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid server url argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (query == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid query argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (quality_issue_report == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid quality_issue_report argument (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    if (parse_serverurl(serverurl, &host, &port, &path) != 0)
    {
        return -1;
    }

    body = create_soap_request(username, password, query);
    if (body == NULL)
    {
        free(host);
        free(path);
        return -1;
    }

    length = strlen(body);
    if (strlen(path) + strlen(USER_AGENT) + strlen(host) + 128 > 1024)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "header of request message to server too long (%s:%u)", __FILE__,
                       __LINE__);
        free(body);
        free(host);
        free(path);
        return -1;
    }

    fd = connect_to_server(host, port);
    if (fd == -1)
    {
        free(body);
        free(host);
        free(path);
        return -1;
    }

#ifndef WIN32
    /* make socket non-blocking */
    fcntl(fd, F_SETFL, O_NONBLOCK);
#endif

    /* send request */
    sprintf(buffer, "POST %s HTTP/1.0\r\n%s\r\nHost: %s\r\nConnection: close\r\n"
            "Content-Type: application/soap+xml\r\nContent-Length: %d\r\n\r\n", path, USER_AGENT, host, length);
    if (qiap_option_debug)
    {
        printf("------------- REQUEST -------------\n");
        fwrite(buffer, strlen(buffer), 1, stdout);
        fwrite(body, length, 1, stdout);
    }
    send(fd, buffer, strlen(buffer), 0);
    send(fd, body, length, 0);
    free(body);
    free(host);
    free(path);

    if (qiap_handle_soap_response(fd, quality_issue_report) != 0)
    {
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}
