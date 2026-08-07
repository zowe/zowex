import com.ibm.security.auth.OS390ThreadSubject;
import com.ibm.security.auth.UsernamePrincipal;
import javax.security.auth.Subject;
import java.util.concurrent.Callable;
import java.security.Principal;

/**
 * Minimal working example to test the OS390ThreadSubject.callAs() method.
 * https://www.ibm.com/docs/en/semeru-runtime-ce-z/25.0.0?topic=jaas-frequently-asked-questions
 * https://www.ibm.com/docs/en/semeru-runtime-ce-z/25.0.0?topic=jaas-os390threadsubject
 *
 * Requires to be compiled on z/OS with Java 25. This source file must be in UTF-8 encoding.
 *
 * Compile and run:
 * $ javac OS390Tester.java
 * $ java -cp . OS390Tester user1
 *
 */

public class OS390Tester {

    public static void main(String[] args) throws Exception{
        if (args.length < 1) {
            System.out.println("Usage: java OS390Tester <target_username>");
            return;
        }

        final String targetUser = args[0];

        System.out.println("Starting test for user: " + targetUser);
        System.out.println("Identity before: " + System.getProperty("user.name"));

        callAs(targetUser, () -> {
            System.out.println("Identity during callAs: " + System.getProperty("user.name"));
            return null;
        });

        System.out.println("Identity after: " + System.getProperty("user.name"));
        System.out.println("SUCCESS!");

    }

    public static void callAs(String username, Callable<Void> action) throws Exception{
        Subject subject = new Subject();
        Principal principal = new UsernamePrincipal(username);
        subject.getPrincipals().add(principal);

        OS390ThreadSubject.callAs(subject, action);
    }
}