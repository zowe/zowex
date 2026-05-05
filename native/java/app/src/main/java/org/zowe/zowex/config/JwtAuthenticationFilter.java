package org.zowe.zowex.config;

import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

import org.springframework.security.authentication.UsernamePasswordAuthenticationToken;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;
import org.zowe.apiml.zaasclient.exception.ZaasClientException;
import org.zowe.apiml.zaasclient.service.ZaasClient;

import java.io.IOException;

@Component
public class JwtAuthenticationFilter extends OncePerRequestFilter {

    private final ZaasClient zaasClient;

    public JwtAuthenticationFilter(ZaasClient zaasClient) {
        this.zaasClient = zaasClient;
    }

    @Override
    protected void doFilterInternal(HttpServletRequest request, HttpServletResponse response, FilterChain filterChain)
            throws ServletException, IOException {

        final String authHeader = request.getHeader("Authorization");

        if (authHeader == null || !authHeader.startsWith("Bearer ")) {
            filterChain.doFilter(request, response);
            return;
        }

        final String jwt = authHeader.substring(7);

        try {
            var tokenResult = zaasClient.query(jwt);

            // Must use the 3-arg constructor to create an authenticated token.
            // Calling setAuthenticated(true) on a 2-arg token throws
            // IllegalArgumentException.
            var authentication = new UsernamePasswordAuthenticationToken(
                    tokenResult.getUserId(),
                    jwt,
                    java.util.Collections.emptyList());
            SecurityContextHolder.getContext().setAuthentication(authentication);
        } catch (ZaasClientException e) {
            System.out.println("Error: " + e.getMessage());
            
            // Validation failed. Clear the context to be safe.
            SecurityContextHolder.clearContext();
            
            // Set the response status to 401 Unauthorized
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            response.setContentType("application/json");
            
            // Safely escape the message (basic escaping for JSON)
            String errorMessage = e.getMessage() != null ? e.getMessage().replace("\"", "\\\"") : "Authentication failed";
            
            // Write the exception message to the response body
            response.getWriter().write("{\"error\": \"" + errorMessage + "\"}");
            response.getWriter().flush();
            
            // Return immediately so the request doesn't continue down the filter chain
            return;
        }

        // Continue the filter chain. If authentication was not set, Spring Security
        // will block the request before it reaches the controller.
        filterChain.doFilter(request, response);
    }
}
