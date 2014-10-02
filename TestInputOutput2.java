public class TestInputOutput2
{
	public static void main(String[] args) {
		printOne();
		printOne();
		printTwo();
                String[] inStrs = {"1234", "5678"};
                String   inStr = "99";
                TestDataInput2 in = new TestDataInput2(inStrs, inStr);
                TestDataOutput2 out = AddTestToInputObj2(in);
                String[] outStrs = out.getOutStrs();
                System.out.println(outStrs[0]);
                System.out.println(outStrs[1]);
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

	public static String AddTestToInputObj(TestDataInput in) {
                TestDataOutput out =  new TestDataOutput();
		out.outInt1 = in.inInt1+10;
		out.outStr1 = "Test:".concat(in.inStr1).concat(String.format("/%d/", in.inInt1));
		out.outStr2 = "Test:".concat(in.inStr2).concat(String.format("/%d/", out.outInt1));
                return out.outStr2;
	}

	public static TestDataOutput2 AddTestToInputObj2(TestDataInput2 in) {
                TestDataOutput2 out =  new TestDataOutput2();
		out.outStr = "Test:".concat(in.inStr);
                out.outStrs = new String[2];
		out.outStrs[0] = "Test:".concat(in.inStrs[0]);
		out.outStrs[1] = "Test:".concat(in.inStrs[1]);
                return out;
	}
}

