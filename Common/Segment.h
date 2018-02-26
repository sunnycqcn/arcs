#ifndef _SEGMENT_H_
#define _SEGMENT_H_ 1

#include "Common/Barcode.h"

#include <cassert>
#include <limits>
#include <string>
#include <utility>

#define MIDDLE_SEGMENT unsigned(-1)

typedef unsigned SegmentIndex;
typedef unsigned Position;

/** One segment of a contig. */
typedef std::string ContigName;
typedef std::pair<ContigName, SegmentIndex> Segment;

/** Hash a Segment. */
struct HashSegment {
    size_t operator()(const Segment& key) const {
        return std::hash<ContigName>()(key.first)
            ^ std::hash<SegmentIndex>()(key.second);
    }
};

/** Map from barcode index => read pair count */
typedef std::map<BarcodeIndex, unsigned> BarcodeToCount;
typedef typename BarcodeToCount::const_iterator BarcodeToCountConstIt;

/** Map from contig segment => barcode indices / read pair counts */
typedef std::unordered_map<Segment, BarcodeToCount, HashSegment>
    SegmentToBarcode;
typedef typename SegmentToBarcode::const_iterator SegmentToBarcodeConstIt;

/** Perform calculations related to contig segments */
class SegmentCalc
{
public:

    SegmentCalc(unsigned segmentSize) : m_segmentSize(segmentSize) {}

    /**
    * Return the index of the contig segment containing the given
    * (one-based) sequence position. If the sequence length
    * is not evenly divisible by the region length,
    * the "remainder" region placed in the middle of the sequence.
    * Return std::numeric_limits<unsigned>::max() if the given position
    * falls within the middle remainder region.
    */
    unsigned index(unsigned pos, unsigned l) const
    {
        /* input should be one-based position, as in SAM format */
        assert(pos > 0);
        assert(pos <= l);
        assert(m_segmentSize <= l / 2);

        /* translate to zero-based pos */
        pos--;

        if (l % m_segmentSize == 0) {
            /* sequence length is perfectly divisible by segment length */
            return pos / m_segmentSize;
        }

        unsigned index;
        if (pos < l / 2) {
            /* pos is in left half of seq */
            index = pos / m_segmentSize;
            if (index >= segmentsPerHalf(l)) {
                /* pos is in remainder middle seg */
                return MIDDLE_SEGMENT;
            }
        } else {
            /* pos is in right half of seq */
            index = (pos - remainder(l)) / m_segmentSize;
            if (index < segmentsPerHalf(l)) {
                /* pos is in middle remainder seg */
                return MIDDLE_SEGMENT;
            }
        }
        return index;
    }

    /**
     * Return the segment index range (first and last segment indices)
     * containing the giving (1-based) alignment coordinate range.
     */
    std::pair<std::pair<SegmentIndex, SegmentIndex>, bool>
        indexRange(Position start, Position end, unsigned l)
    {
        if (l < m_segmentSize * 2) {
            return std::make_pair(std::make_pair(0, 0), false);
        }

        std::pair<SegmentIndex, SegmentIndex> range =
            std::make_pair(index(start, l), index(end, l));

        if (range.first == MIDDLE_SEGMENT && range.second == MIDDLE_SEGMENT) {
            return std::make_pair(range, false);
        } else if (range.first == MIDDLE_SEGMENT) {
            range.first = segmentsPerHalf(l);
            return std::make_pair(range, true);
        } else if (range.second == MIDDLE_SEGMENT) {
            range.second = segmentsPerHalf(l) - 1;
        }

        return std::make_pair(range, true);
    }

    /**
     * Return the number of segments contained in each
     * half of the sequence.  Precondition: Sequence length
     * is not evenly divisible by segment length.
     */
    unsigned segmentsPerHalf(unsigned l) const
    {
        assert(m_segmentSize <= l / 2);
        assert(l % m_segmentSize > 0);
        return l / 2 / m_segmentSize;
    }

    /** Return the number of segments in a sequence of the given length */
    unsigned segments(unsigned l) const
    {
        assert(m_segmentSize <= l / 2);
        if (l % m_segmentSize == 0) {
            /* sequence length is perfectly divisible by segment length */
            return l / m_segmentSize;
        }
        return segmentsPerHalf(l) * 2;
    }

    /**
     * Return one-based start position of the segment
     * with the given index
     */
    unsigned start(unsigned l, unsigned index) const
    {
        assert(m_segmentSize <= l / 2);
        if (l % m_segmentSize == 0) {
            return index * m_segmentSize + 1;
        }

        unsigned segsPerHalf = segmentsPerHalf(l);
        if (index < segsPerHalf) {
            return index * m_segmentSize + 1;
        } else {
            unsigned rightIndex = index - segsPerHalf;
            return segsPerHalf * m_segmentSize
                + remainder(l) + rightIndex * m_segmentSize + 1;
        }
    }

    /**
     * Return length of remainder area in middle of sequence,
     * which is not coverage by any segment
     */
    unsigned remainder(unsigned l) const
    {
        assert(m_segmentSize <= l / 2);
        if (l % m_segmentSize == 0) {
            return 0;
        }
        assert(l > m_segmentSize * segmentsPerHalf(l) * 2);
        return l - m_segmentSize * segmentsPerHalf(l) * 2;
    }

protected:

    /** length of a contig segment in base pairs */
    unsigned m_segmentSize;
};

#endif