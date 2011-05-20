/**
 * This class parses a given form description json file and prints information
 * about the form. This class provides an example of how to write Java code
 * that interacts with the json form.
 */

import java.awt.Point;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.Reader;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;


public class Json {
	
	// Keep track of indentation as objects are printed. 
	private static int INDENTATION = -1;
	
	/**
	 * Opens the file given as a command line argument and parses the file.
	 * @param args
	 * @throws FileNotFoundException
	 */
	public static void main(String[] args) throws FileNotFoundException {
		String filename = args[0];
		File file = new File(filename);
		Reader reader = new FileReader(file);
		JsonParser parser = new JsonParser();
		JsonElement segment = parser.parse(reader);
		
		parseSegment(segment);
	}
	
	/**
	 * Recursively parses a single object within the Json. Only parses objects
	 * that have the expected types: "segment" and "bubble_block."
	 * @param element The root element of the Json.
	 */
	public static void parseSegment(JsonElement element) {
		INDENTATION++;
		JsonObject segment = element.getAsJsonObject();
		String type = segment.get("dataType").getAsString();
		// Parse this segment.
		if (type.equals("segment")) {
			printSegment(segment);
			Iterator<JsonElement> iter = segment.get("segments").getAsJsonArray().iterator();
			// Parse all segments within this segment.
			while (iter.hasNext()) {
				JsonElement e = iter.next();
				parseSegment(e);
			}
		// Parse as bubble_block
		} else if (type.equals("bubble_block")) {
			printBubbleBlock(segment);
		} else {
			System.out.println("Unexpected segment type: " + type);
		}
		INDENTATION--;
	}
	
	/**
	 * Obtains attributes within a given segment and prints them out.
	 * This may be useful when trying to generate test code; just change the
	 * format string.
	 * 
	 * @param segment The segment to be parsed. It should have name, width,
	 * height, x and y attributes.
	 */
	public static void printSegment(JsonObject segment) {
		String name = segment.getAsJsonObject().get("name").getAsString();
		int width = segment.getAsJsonObject().get("width").getAsInt();
		int height = segment.getAsJsonObject().get("height").getAsInt();
		int x = segment.getAsJsonObject().get("x").getAsInt();
		int y = segment.getAsJsonObject().get("y").getAsInt();
		
		// To generate tests, consider changing this to 
		// String format = "cpp_method_to_call(\"%s\", %d, %d, %d, %d);";
		String format = "%s, %d, %d, %d, %d";
		
		printWithIndent(String.format(format, name, width, height, x, y));
	}
	
	/**
	 * Prints the bubble block: the type and the coordinates of all the bubble
	 * points. 
	 * @param segment
	 */
	public static void printBubbleBlock(JsonObject segment) {
		printWithIndent("bubble_block");
		
		// Bubbles are serialized as a list. Obtain the iterator from the list.
		JsonArray coordinates = segment.get("bubbles").getAsJsonArray();
		Iterator<JsonElement> iter = coordinates.iterator();
		
		// Collect all bubble points.
		Set<Point> points = new HashSet<Point>();
		while(iter.hasNext()) {
			int x = iter.next().getAsInt();
			int y = iter.next().getAsInt();
			points.add(new Point(x, y));
		}
		
		// Print the list of points.
		printWithIndent(Arrays.deepToString(points.toArray()));
	}
	
	/**
	 * Prints the given string at the given indentation level. Each level of
	 * indentation will consist of one tab character.
	 * @param i The level of indentation.
	 * @param s The string to print.
	 */
	private static void printWithIndent(String s) {
		for (int i = 0; i < INDENTATION; i++) {
			System.out.print("    ");
		}
		System.out.println(s);
	}
}
