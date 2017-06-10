package com.bgate.s2;

import java.util.ArrayList;
import java.util.List;

import com.google.common.geometry.S2Cell;
import com.google.common.geometry.S2CellId;
import com.google.common.geometry.S2CellUnion;
import com.google.common.geometry.S2LatLng;
import com.google.common.geometry.S2Loop;
import com.google.common.geometry.S2Point;
import com.google.common.geometry.S2Polygon;
import com.google.common.geometry.S2RegionCoverer;

public class S2Helper 
{
	//=======================================
	//			STATIC FIELDS
	//=======================================
	public static final int MIN_LEVEL = 12;
	public static final int MAX_LEVEL = 12;
	public static final int MAX_CELLS = Integer.MAX_VALUE;
	
	/**
	 * radius of the Earth, calculated in meter
	 */
	private static final double EARTH_RADIUS = 6378137;

	//=======================================
	//			STATIC METHODS
	//=======================================
	/**
	 * create an S2Helper object with default arguments :
	 * <pre>
	 * minLevel = 12
	 * maxLevel = 12
	 * maxCells = {@link Integer.MAX_VALUE}
	 * </pre>
	 * @return S2Helper object
	 */
	public static final S2Helper createDefault()
	{
		return create(MIN_LEVEL, MAX_LEVEL, MAX_CELLS);
	}

	/**
	 * 
	 * @param minLevel : minimum level of s2 geometry hierarchical decomposition
	 * @param maxLevel : maximum level of s2 geometry hierarchical decomposition
	 * @param maxCells : maximum amount of cells would be generated in union
	 * @return S2Helper object
	 */
	public static final S2Helper create(final int minLevel, final int maxLevel, final int maxCells)
	{
		final S2Helper helper = new S2Helper();
		if (helper.init(minLevel, maxLevel, maxCells)) return helper;
		return null;
	}

	//=======================================
	//			FIELDS
	//=======================================
	/**
	 * an S2RegionCoverer used to construct covering
	 */
	private S2RegionCoverer _coverer;

	//=======================================
	//			CONSTRUCTORS
	//=======================================	
	private S2Helper()
	{

	}

	//=======================================
	//			METHODS
	//=======================================
	private boolean init(final int minLevel, final int maxLevel, final int maxCells)
	{
		_coverer = new S2RegionCoverer();
		_coverer.setMinLevel(minLevel);
		_coverer.setMaxLevel(maxLevel);
		_coverer.setMaxCells(maxCells);

		return true;
	}

	/**
	 * 
	 * @param latlng : check point
	 * @param xmeter : offset in horizontal, calculated in meter
	 * @param ymeter : offset in vertical, calculated in meter
	 * @return
	 */
	private final S2LatLng translate(final S2LatLng latlng, final double xmeter, final double ymeter)
	{
		//Coordinate offsets in radians
		final double dLat = ymeter/EARTH_RADIUS;
		final double dLon = xmeter/(EARTH_RADIUS*Math.cos(latlng.latRadians()));

		//OffsetPosition, decimal degrees
		double latO = latlng.latDegrees() + dLat * 180/Math.PI;
		double lonO = latlng.lngDegrees() + dLon * 180/Math.PI;
		
		while(lonO > 180) lonO -= 360;
		while(latO > 90) latO -= 180;
		
		while(lonO < -180) lonO += 360;
		while(latO < -90) latO += 180;
		
		return S2LatLng.fromDegrees(latO, lonO);
	}

	/**
	 * 
	 * @param latlng : check point
	 * @param range : radius of circle surrounding check point, calculated in meter
	 * @return an union of cells that covering the region constructed from check point and range
	 */
	public final synchronized S2CellUnion getCellUnion(final S2LatLng latlng, final double range)
	{
		final ArrayList<S2Loop> loops = new ArrayList<S2Loop>();
		final ArrayList<S2Point> points = new ArrayList<S2Point>();
		for(int i = 0; i < 360; i += 20)
		{
			final double x = range * Math.cos(i * Math.PI / 180);
			final double y = range * Math.sin(i * Math.PI / 180);
			points.add(translate(latlng, x , y).toPoint());
		}
		loops.add(new S2Loop(points));
		final S2Polygon poly = new S2Polygon(loops);
		return _coverer.getCovering(poly);
	}
	
	/**
	 * 
	 * @param latlng1 : latitude and longitude of check point 1
	 * @param latlng2 : latitude and longitude of check point 2
	 * @return distance between two points references by latlng1 and latlng2
	 */
	private final double distance(final S2LatLng latlng1, final S2LatLng latlng2)
	{
		final double dlat = (latlng2.latDegrees() - latlng1.latDegrees()) * Math.PI / 180;
		final double dlon = (latlng2.lngDegrees() - latlng1.lngDegrees()) * Math.PI / 180;

		final double y = dlat * EARTH_RADIUS;
		final double x = dlon * EARTH_RADIUS * Math.cos(latlng1.latRadians());
		
		return Math.sqrt(y * y + x * x);
	}
	
	/**
	 * 
	 * @param latlngs : list of check points
	 * @return distance through all check points, calculated in meter
	 */
	public final double getDistance(final S2LatLng... latlngs)
	{
		if(latlngs.length == 1) return 0;
		
		double result = 0;
		for(int i = 0; i < latlngs.length - 1; i++) 
			result += distance(latlngs[i], latlngs[i+1]);
		
		return result;
	}
	
	/**
	 * 
	 * @param latlngs : list of check points
	 * @return distance through all check points, calculated in meter
	 */
	public final double getDistance(final List<S2LatLng> latlngs)
	{
		if(latlngs.size() == 1) return 0;
		
		double result = 0;
		
		for(int i = 0, size = latlngs.size(); i < size - 1; i++) 
			result += distance(latlngs.get(i), latlngs.get(i+1));
		
		return result;
	}
	
	public final ArrayList<S2CellId> getCellIds(S2CellUnion union)
	{
		ArrayList<S2CellId> p = new ArrayList<S2CellId>();
		
		for(S2CellId id : union.cellIds()) {
		  if(id.level() == _coverer.maxLevel()) {
			  p.add(new S2CellId(id.id()));
		  } else {
			  S2CellId child = id.childBegin();
			  S2CellId endchild = id.childEnd();
			  S2CellId t = child;
			  
			  while(true) {
				  if(t.id() == endchild.id()) break;
				  p.add(new S2CellId(t.id()));
				  t = t.next();
			  }
			  
		  }
		}		
		
		return p;
	}
	
	public final S2CellId getCellId(double lat, double lng, int level)
	{
		S2CellId p = S2CellId.fromLatLng(S2LatLng.fromDegrees(lat, lng));
		if(p.level() > level) return p.parent(level);
		return p;
	}
	
	public static void main(String[] args)
	  {
		  S2LatLng latlng = S2LatLng.fromDegrees(21.014837, 105.849914);
		  double range = 200;
		  
		  S2Helper helper = S2Helper.create(16, 16, 999999);
		  
		  S2CellUnion union = helper.getCellUnion(latlng, range);
		  
		  helper.getCellIds(union);
		  
//		  for(S2CellId id : union.cellIds()) {
//			  System.out.println("---------");
//			  System.out.println(id.id() + ", " + id.level());
//			  S2CellId child = id.childBegin();
//			  S2CellId endchild = id.childEnd();
//			  S2CellId t = child;
//			  while(true) {
//				  if(t.id() == endchild.id()) break;
//				  System.out.println("child : " + t.id() + ", " + t.level());
//				  t = t.next();
//			  }
//		  }
		  if(union.contains(S2LatLng.fromDegrees(21.004018, 105.850031).toPoint()))
		  {
			  System.out.println("contain points");
		  }
		  
	  }
}
