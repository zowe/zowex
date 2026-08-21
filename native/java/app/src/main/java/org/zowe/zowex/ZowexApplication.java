package org.zowe.zowex;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.zowe.apiml.enable.EnableApiDiscovery;

import org.springframework.context.annotation.Import;
import org.zowe.apiml.zaasclient.config.DefaultZaasClientConfiguration;

@SpringBootApplication(scanBasePackages = {"org.zowe.zowex"})
@EnableApiDiscovery
@Import(DefaultZaasClientConfiguration.class)
public class ZowexApplication {
    public static void main(String[] args) {
        try {
            SpringApplication.run(ZowexApplication.class, args);
        } catch (Throwable t) {
            System.err.println("========== FATAL STARTUP ERROR ==========");
            t.printStackTrace();
            System.err.println("=========================================");
            System.exit(1);
        }
    }
}
