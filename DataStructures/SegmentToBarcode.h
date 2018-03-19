#ifndef _BARCODE_INDEX_H_
#define _BARCODE_INDEX_H_ 1

#include "Common/SetUtil.h"
#include "DataStructures/Barcode.h"
#include "DataStructures/Segment.h"

#include <map>
#include <unordered_map>

/** Maps barcode => number of read mappings */
typedef std::map<BarcodeIndex, unsigned> BarcodeToCount;
typedef typename BarcodeToCount::const_iterator BarcodeToCountConstIt;

/** Maps contig segment => barcodes / mapping counts */
typedef std::unordered_map<Segment, BarcodeToCount, HashSegment>
	SegmentToBarcode;
typedef typename SegmentToBarcode::const_iterator SegmentToBarcodeConstIt;
typedef typename SegmentToBarcode::iterator SegmentToBarcodeIt;

/** get barcodes for the given segment and append to `out` */
static inline void addBarcodes(const Segment& segment,
	const SegmentToBarcode& segmentToBarcode,
	std::vector<BarcodeIndex>& out)
{
	SegmentToBarcodeConstIt segmentIt = segmentToBarcode.find(segment);
	if (segmentIt == segmentToBarcode.end())
		return;
	for (const auto& rec : segmentIt->second)
		out.push_back(rec.first);
}

/** compute barcode Jaccard score between two segments */
static inline double jaccard(const Segment& segment1, const Segment& segment2,
	const SegmentToBarcode& segmentToBarcode)
{
	BarcodeList barcodes1, barcodes2;
	barcodes1.reserve(1000);
	barcodes2.reserve(1000);

	addBarcodes(segment1, segmentToBarcode, barcodes1);
	addBarcodes(segment2, segmentToBarcode, barcodes2);

	return jaccard(barcodes1, barcodes2);
}

#endif