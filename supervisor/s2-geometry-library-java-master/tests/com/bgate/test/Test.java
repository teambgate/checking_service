package com.bgate.test;

import com.bgate.s2.S2Helper;
import com.google.common.geometry.S2CellUnion;
import com.google.common.geometry.S2LatLng;


public class Test 
{
	
	  public static void main(String[] args)
	  {
		  S2LatLng latlng = S2LatLng.fromDegrees(21.0066626,105.8484434);
		  double range = 200;
		  
		  S2CellUnion union = S2Helper.createDefault().getCellUnion(latlng, range);
		  
		  if(union.contains(S2LatLng.fromDegrees(21.004018, 105.850031).toPoint()))
		  {
			  System.out.println("contain points");
		  }
		  
	  }
	
}
