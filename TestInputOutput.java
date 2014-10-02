/*class TestDataInput
{
        public String inStr1, inStr2;
        public Integer inInt1;
        public TestDataInput()
        {
              inStr1 = "";
              inStr2 = "";
              inInt1 = 0;
        }
    public TestDataInput(String str1, String str2, Integer int1)
    {
        this.inStr1 = str1;
        this.inStr2 = str2;
        this.inInt1 = int1;
    }
}

class TestDataOutput
{
        public String outStr1, outStr2;
        public Integer outInt1;
        TestDataOutput()
        {
              outStr1 = "";
              outStr2 = "";
              outInt1 = 0;
        }
        public String getOutStr1()
        {
              return outStr1;
        }
}
*/
public class TestInputOutput
{
	public static void main(String[] args) {
		printOne();
		printOne();
		printTwo();
                TestDataInput in = new TestDataInput("1234", "5678", 9);
                TestDataOutput out = AddTestToInputObj2(in);
                System.out.println(out.getOutStr1());
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

	public static TestDataOutput AddTestToInputObj2(TestDataInput in) {
System.out.println("111111");
                TestDataOutput out =  new TestDataOutput();
		out.outInt1 = in.inInt1+10;
		out.outStr1 = "Test:".concat(in.inStr1).concat(String.format("/%d/", in.inInt1));
		out.outStr2 = "Test:".concat(in.inStr2).concat(String.format("/%d/", out.outInt1));
                return out;
	}
}

