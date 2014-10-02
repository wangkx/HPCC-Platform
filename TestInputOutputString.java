
/* CallingMethodsInSameClass.java
 *
 * illustrates how to call static methods a class
 * from a method in the same class
 */

public class TestInputOutputString
{
	public static void main(String[] args) {
		printOne();
		printOne();
		printTwo();
	}

	public static void printOne() {
                System.out.println(AddTestToInput("Hello World"));
	}

	public static void printTwo() {
		printOne();
		printOne();
	}

	public static String AddTestToInput(String inStr) {
		String outStr = "Test:".concat(inStr);
                return outStr;
	}
}

