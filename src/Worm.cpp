/*
  Name:         Worm.cpp (implementation)
  Author:       Kip Warner (Kip@TheVertigo.com)
  Description:  Worm class...
*/

// Includes...
#include "Worm.h"
#include <new>
#include <cmath>
#include <cfloat>
#include <cassert>

// Worm construction requires to just know it's contour and a bit of information about the image it rests on...
inline Worm::Worm(CvSeq const &Contour, IplImage const &Image)
    : pStorage(cvCreateMemStorage(0)),
      pContour(NULL),
      unUpdates(0),
      dArea(0.0f),
      dLength(0.0f), 
      dWidth(0.0f)
{    
    // Allocatation of base storage for contour and anything else we need failed...
    if(!pStorage)
        throw bad_alloc;

    // Update the worm's metrics based on the contour...
    DiscoverMetrics(Contour, Image);
}

// Adjust the distance of the second vertex by the given distance along the radial... 
inline void Worm::AdjustDirectedLineSegmentLength(LineSegment &A, float fLength) const
{
    // Create vector from directed line segment...
    CvPoint Vector(A.second.x - A.first.x, A.second.y - A.first.y);
    
    // Compute angle of vector in degrees, then convert to radians...
    float const fThetaRadians = cvFastArctan(Vector.x, Vector.y) * (Pi / 180);
    
    // Adjust vector length to given...
    Vector.x = fLength * cos(fThetaRadians);
    Vector.y = fLength * sin(fThetaRadians);
    
    // Translate back again...
    A.second.x = A.first.x + Vector.x;
    A.second.y = A.first.y + Vector.y;
}       

// Best guess of the area, considering everything we've seen thus far...
inline float const &Worm::Area() const
{
    // Return it...
    return &fArea;
}

// Is directed line segment Start->Second clockwise (> 0), counterclockwise (< 0), or collinear with respect to
//  the directed line segment Start->First? θ(1)
inline int Worm::Direction(CvPoint const Start, CvPoint const First, CvPoint const Second) const
{
    // Calculate the cross product, but do it with both vectors translated back to the origin to make it work...
    return (((First.x - Start.x) * (Second.y - Start.y)) - 
            ((Second.x - Start.x) * (First.y - Start.y)));
}

// Discover the worm's metrics based on its new contour... (area, length, width, et cetera)
inline void Worm::Discover(CvSeq const &NewContour, IplImage const &Image)
{
    // Forget the old contour if we have one yet. This works by restoring the old contour sequence blocks to 
    //  the base storage pool. This is θ(1) running time, usually...
    if(pContour)
        cvClearSeq(pContour);

    // Clone the new contour sequence into our storage pool...
    pContour = cvCloneSeq(&NewContour, pStorage);

    // Remember that how many times we have updated, which we need for calculating arithmetic means...
  ++unUpdates;

    // Update the approximate area from the area calculated in *this* image...
    UpdateArea(fabs(cvContourArea(pContour));

    // Update the approximate length from the length calculated in *this* image. The length is about half
    //  the perimeter all the way around the worm...
    float const fLengthAtThisMoment = cvArcLength(pContour, CV_WHOLE_SEQ, true) / 2.0;
    UpdateLength(fLengthAtThisMoment);

    // Find both ends... (head and tail)
 
        // Find an end, either will do... θ(n)
        unsigned int const unMysteryVertexIndexEnd = PinchShiftForAnEnd(pContour); 
        
        // Find the other end of the worm which must be approximately the length of the worm away... O(n)
        unsigned int const unOtherMysteryVertexIndexEnd = 
            FindNearestVertexIndexByPerimeterLength(unMysteryVertexIndexEnd, fLengthAtThisMoment);
        
        // Make a reasonably intelligent guess as to which end is which, based only on *this* image alone...
            
            // The first end we found was probably the head...
            if(IsFirstProbablyHeadByCloisteredCheck(unMysteryVertexIndex, unOtherMysteryVertexIndexEnd, Image))
                UpdateHeadAndTail(unMysteryVertexIndex, unOtherMysteryVertexIndexEnd);
            
            // Nope, it was the other way around...
            else
                UpdateHeadAndTail(unOtherMysteryVertexIndexEnd, unMysteryVertexIndex);

    // Finally calculate the width of the worm...
    
        // Let us measure from approximately half way up the worm from either end, 1/2 the total length... O(n)
        unsigned int const unVertexIndexOfMiddleSideA =
            FindNearestVertexIndexByPerimeterLength(unMysteryVertexIndex, fLengthAtThisMoment / 2.0f);
        
        // Now find the middle on the other side by going the other way half the length... O(n)
        unsigned int const unVertexIndexOfMiddleSideB =
            FindNearestVertexIndexByPerimeterLength(unMysteryVertexIndex, -1.0f * fLengthAtThisMoment / 2.0f);
        
        // The distance between the two points is approximately the width of the worm...
        UpdateWidth(DistanceBetweenTwoPoints(GetVertex(unVertexIndexOfMiddleSideA), 
                                             GetVertex(unVertexIndexOfMiddleSideB))); 
}

// Calculate the distance between the midpoints of two segments... θ(1)
inline float Worm::DistanceBetweenLineSegments(LineSegment const &A, LineSegment const &B) const
{
    // Just measure the length of the imaginary line segment joining both segment's middles...
    return LengthOfLineSegment(CvPoint((A.second.x - A.first.x) / 2, (A.second.y - A.first.y) / 2), 
                               CvPoint((B.second.x - B.first.x) / 2, (B.second.y - B.first.y) / 2));
}

// Calculate the absolute distance between two points...
inline float Worm::DistanceBetweenTwoPoints(CvPoint const &First, CvPoint const &Second) const
{
    // Return it...
    return cvSqrt(pow(Second.x - First.x, 2) + pow(Second.y - First.y, 2));
} 

// Find the vertex on the contour the given length away, starting in increasing order... O(n)
inline unsigned int const Worm::FindNearestVertexIndexByPerimeterLength(unsigned int const &unStartVertex, 
                                                                        float const &fPerimeterLength) const
{
    /* TODO: We can do much better than this right down to logarithmic time by using a skip list 
             some how. Deal with it later after we're sure it works properly at all. */

    // Variables...
    register float  fDistanceWalkedAccumulator  = 0.0f;
    
    // Easy case...
    if(fPerimeterLength == 0.0f)
        return unStartVertex;

    // Keep walking along the contour in the requested direction until we've gone far enough...
    for(unsigned int unCurrentVertexIndex = unStartIndex; 
        fDistanceWalkedAccumulator < fPerimeterLength; 
        (fPerimeterLength > 0) ? (unCurrentVertexIndex = GetNextVertexIndex(unCurrentVertexIndex)) 
                               : (unCurrentVertexIndex = GetPreviousVertexIndex(unCurrentVertexIndex)))
    {
        // Remember how far we've walked...
        fDistanceWalkedAccumulator += 
            DistanceBetweenTwoPoints(GetVertex(unCurrentVertexIndex), 
                                     GetVertex(GetNextVertexIndex(unCurrentVertexIndex)));
    }
    
    // We've gone far enough. Let's be modest rather than liberal and underestimate rather than over...
    return GetPreviousVertexIndex(unCurrentVertexIndex);
}

// Generate orthogonal of unit length from middle of given line segment outwards... θ(1)
inline void Worm::GenerateOrthogonalToLineSegment(LineSegment const &A, 
                                                  LineSegment &Orthogonal) const
{
    // The orthogonal start is just the middle of the given line segment since that is where it begins...
    Orthogonal.first.x = (A.second.x - A.first.x) / 2;
    Orthogonal.first.y = (A.second.y - A.first.y) / 2;
    
    // The orthogonal end is a little more complicated...
    
        // Calculate the slope of the given line segment...
        float const fSlope = (A.second.y - A.first.y) / float(A.second.x - A.first.x)
        
            // Given line is perfectly horizontal, correct for divide by zero...
            if(fSlope == 0.0f)
            {
                // End point's x-coordinate will have the same x-coordinate as its own start point's...
                Orthogonal.second.x = Orthogonal.first.x;
                
                // The end point will be directly one unit up from the given line... 
                Orthogonal.second.y = A.first.y + 1;

                // Done...
                return;
            }
        
        // Both require the ratio of the given line's slope to the square root of the sum of its square and one...
        float const fPartialCalculation = fSlope / cvSqrt((fSlope * fSlope) + 1);

        // Now calculate the X and Y coordinates...
        Orthogonal.second.x = (int) (float(A.second.x + A.first.x) / 2.0 + fPartialCalculation);
        Orthogonal.second.y = (int) (float(A.second.y + A.first.y) / 2.0 - fPartialCalculation);
}

// Get the index of the next vertex in the contour after the given index, O(1) average...
inline unsigned int &Worm::GetNextVertexIndex(unsigned int const &unVertexIndex) const
{
    // This should never happen, so we make sure...
    assert(unVertex < pContour->total);
    
    // The next index is just one more than the given - unless at the end where it jumps back to the beginning...
    return (unVertexIndex < pContour->total) ? (unVertexIndex + 1) : 0;
}

// Get the actual vertex of the given vertex index in the contour, O(1) average...
CvPoint const &Worm::GetVertex(unsigned int const &unVertexIndex) const
{
    // This should never happen, so we make sure...
    assert(unVertex < pContour->total);
    
    // Find the actual vertex and return it...
    return (CvPoint const *) cvGetSeqElem(pContour, unVertexIndex);
}

// Get the index of the previous vertex in the contour after the given index, O(1) average...
inline unsigned int &Worm::GetPreviousVertexIndex(unsigned int const &unVertexIndex) const
{
    // This should never happen...
    assert(unVertex < pContour->total);
    
    // The previous index is just one less than the given - unless at the start where it jumps back to the end... 
    return (unVertexIndex == 0) ? (pContour->total - 1) : (unVertexIndex - 1);
}

// Best guess as to the head's position at this moment in time, since it changes...
inline CvPoint &Worm::Head() const
{
    /* TODO: Implement this. */
}

// Can the collinear point be found on the line segment? θ(1)
inline bool Worm::IsCollinearPointOnLineSegment(LineSegment const &A, CvPoint const &CollinearPoint) const
{
    // Found...
    if(((min(A.first.x, A.second.x) <= CollinearPoint.x) && (CollinearPoint.x <= max(A.first.x, A.second.x))) &&
       ((min(A.first.y, A.second.y) <= CollinearPoint.y) && (CollinearPoint.y <= max(A.first.y, A.second.y))))
        return true;
    
    // Not found....
    else
        return false;
}                                                   

// Given only the two vertex indices, *this* image, and assuming they are opposite ends of the worm, 
//  would the first of the two most likely be the head if we had only this image to consider?
inline bool Worm::IsFirstProbablyHeadByCloisteredCheck(unsigned int const &unCandidateHeadVertexIndex,
                                                       unsigned int const &unCandidateTailVertexIndex,
                                                       IplImage const &Image) const
{
    /* TODO: Implement this. Perhaps try summing the YUV luma value in both regions and comparing? */
}

// Check if two line segments intersect... θ(1)
inline bool Worm::IsLineSegmentsIntersect(LineSegment const &A, LineSegment const &B) const
{
    // Calculate the relative orientation of each endpoint with respect to the other segment...
    int nDirection1 = Direction(B.first, B.second, A.first);
    int nDirection2 = Direction(B.first, B.second, A.second);
    int nDirection3 = Direction(A.first, A.second, B.first);
    int nDirection4 = Direction(A.first, A.second, B.second);
    
    /* See pp.934-938 of Cormen et al, 2003, for the verbose explanation of how this works. In short, it
        works by checking for the straddling of line segments... */
        
    // Intersects...
    if(((nDirection1 > 0 && nDirection2 < 0) || (nDirection1 < 0 && nDirection2 > 0)) &&
        (nDirection3 > 0 && nDirection4 < 0) || (nDirection3 < 0 && nDirection4 > 0)))
        return true;
    
    // Intersects...
    else if((nDirection1 == 0) && IsCollinearPointOnLineSegment(LineSegment(B.first, B.second), A.first))
        return true;
    
    // Intersects...
    else if((nDirection2 == 0) && IsCollinearPointOnLineSegment(LineSegment(B.first, B.second), A.second))
        return true;

    // Intersects...
    else if((nDirection3 == 0) && IsCollinearPointOnLineSegment(LineSegment(A.first, A.second), B.first))
        return true;
        
    // Intersects...
    else if((nDirection4 == 0) && IsCollinearPointOnLineSegment(LineSegment(A.first, A.second), B.second))
        return true;
    
    // Every necessary and sufficient condition for two line segments to intersect has not been satisfied...
    else
        return false;
}

// Best guess of the length from head to tail, considering everything we've seen thus far...
inline float const &Worm::Length() const
{
    // Return it...
    return &fLength;
}

// Calculate the length of a line segment... θ(1)
inline float Worm::LengthOfLineSegment(LineSegment const &A) const
{
    // Calculate...
    return cvSqrt(((A.second.x - A.first.x) * (A.second.x - A.first.x)) + 
                  ((A.second.y - A.first.y) * (A.second.y - A.first.y)));
}

// Show some info of what we know about this worm...
inline std::ostream & Worm::operator<<(std::ostream &Output, Worm &RequestedWorm) const
{
    // Output attributes of note...
    Output << "area: "          << dArea    << ", "
           << "length: "        << dLength  << ", "
           << "width: "         << dWidth   << ", "
           << "(pool base: "    << dStorage << ")";
           
    // Return the stream...
    return Output;
}

// Candidate is not similar enough... (see below)
inline bool Worm::operator<(Worm &CandidateWorm) const
{
    // Less than similar...
    if(CandidateWorm != *this)
        return true;
    
    // Similar, so not less than...
    else
        return false;
}

// Candidate meets the minimum required similarity...
inline bool Worm::operator==(Worm &CandidateWorm) const
{
    /* TODO: Perhaps cvMeanShift could be useful here? */
}

// Find the vertex index in the contour sequence that contains either end of the worm... θ(n)
inline unsigned int Worm::PinchShiftForAnEnd() const
{
    // Variables...
    unsigned int const  unStartVertexIndex                                  = 0;
    unsigned int        unCurrentOppositeVertexIndex                        = 0;
    unsigned int        unClosestOppositeVertexIndexFound                   = 0;    
    float               fClosestOppositeVertexDistanceFound                 = Infinity;
    LineSegment         StartingLineSegment(CvPoint(0, 0), CvPoint(0, 0));
    LineSegment         OrthogonalLineSegment(0, 0);

    // We begin by forming a line segment from an arbitrary point on the contour to its neighbour...
    StartingLineSegment.first   = GetVertex(unStartVertexIndex);
    StartingLineSegment.second  = GetVertex(GetNextVertexIndex(unStartVertexIndex));

    // Generate an orthogonal for the starting line segment...
    GenerateOrthogonalToLineSegment(StartingLineSegment, OrthogonalLineSegment);
    
    // Ensure that the orthogonal is directed into the worm, rather than outwards...
    
        // Make it really short so that its other end won't accidentally extend right through the other side...
        AdjustDirectedLineSegmentLength(OrthogonalLineSegment, 0.1f);
        
        // If it isn't pointing into the worm, flip it so that it is...
        if(cvPointPolygonTest(pContour, OrthogonalLineSegment.second, 0) < 0.0f)
            AdjustDirectedLineSegmentLength(OrthogonalLineSegment, -0.1f);

        // The orthogonal directed segment's other side should always be within the worm...
        assert(cvPointPolygonTest(pContour, OrthogonalLineSegment.second, 0) == 1.0f);

    // Extend the directed orthogonal line segment out very far and clip to the very edge of the image...
    AdjustDirectedLineSegmentLength(OrthogonalLineSegment, Infinity);
    cvClipLine(ImageSize, &OrthogonalLineSegment.first, &OrthogonalLineSegment.second);

    // Now find the closest line segment that this orthogonal, which serves us as a guide, pierces on the
    //  other side. Although the worm may be in some peculiar shape, it will not matter since the first 
    //  segment pierced must be somewhere on the other side, though not necessarily directly opposite...
    
        // Start searching beginning with the next neighbour of our starting segment...
        unCurrentOppositeVertexIndex = GetNextVertexIndex(unStartVertexIndex);
        
        // Now go around the creature, looking for those segments that the above pierces...
        while(unStartVertexIndex != GetNextVertexIndex(unCurrentOppositeVertexIndex))
        {
            // The line segment we are going to test...
            LineSegment CandidateLineSegment(GetVertex(unCurrentOppositeVertexIndex), 
                                             GetNextVertex(unCurrentOppositeVertexIndex));
            
            // Ah ha! We have found a segment that intersects the orthogonal...
            if(IsLineSegmentsIntersect(OrthogonalLineSegment, CandidateLineSegment))
            {
                // How far away were they?
                float const fDistanceBetweenMiddleOfLineSegments = 
                    DistanceBetweenLineSegments(StartingLineSegment, CandidateLineSegment);
                
                // Was this distance closer than anything we've encountered thus far?
                if(fDistanceBetweenMiddleOfLineSegments < fClosestOppositeVertexDistanceFound)
                {
                    // Make a note of where it was and how far away it was...
                    unClosestOppositeVertexIndexFound   = unCurrentOppositeVertexIndex;
                    fClosestOppositeVertexDistanceFound = fDistanceBetweenMiddleOfLineSegments;
                }
            }
            
            // Let's move to the next segment and try that one...
            unCurrentOppositeVertexIndex = GetNextVertexIndex(unCurrentOppositeVertexIndex);
        }

    // We now have both the start and opposite vertex of the worm. This is all we need now...
    unsigned int unVertexIndexSideA = unStartVertexIndex;
    unsigned int unVertexIndexSideB = unClosestOppositeVertexIndexFound;

    // Keep shifting the two points along the worm's body. When the two vertices finally coalesce into one
    // (they have the same coordinates), we have found an end...
    while(unVertexIndexSideA != unVertexIndexSideB)
    {
        // Since the vertex density per area of space may differ on either side of the worm, we cannot simply
        //  shuffle both sides together at the same rate. To solve this problem, assuming one of the sides
        //  has to move, move the side that keeps the closure distance between the two minimal...
        
            // If we shift the vertex of side A, how far apart would the two be?
            float const fDistanceBetweenIfShiftA = 
                DistanceBetweenTwoPoints(GetVertex(GetPreviousVertexIndex(unVertexIndexSideA)),
                                         GetVertex(unVertexIndexSideB)));
                                                                           
            // If we shift the vertex of side B, how far apart would the two be?
            float const fDistanceBetweenIfShiftB = 
                DistanceBetweenTwoPoints(GetVertex(unVertexIndexSideA),
                                         GetVertex(GetNextVertexIndex(unVertexIndexSideB)));
        
            // Shifting side A is the best choice to make, so do it...
            if(fDistanceBetweenIfShiftA <= fDistanceBetweenIfShiftB)
                unVertexIndexSideA = GetNextVertex(unVertexIndexSideA);
        
            // Shifting side B is the best choice to make, so do it...
            else
                unVertexIndexSideB = GetNextVertex(unVertexIndexSideB);
    }

    // The index of the vertex of the head / tail is either vertex, since they are the same...
    return unVertexSideA;
}

// Rotate a line segment about a point counterclockwise by an angle...
inline void Worm::RotateLineSegmentAboutPoint(LineSegment &LineToRotate, CvPoint const &Origin, 
                                              float const &fRadians) const
{
    // Apply rotation about the specified origin for the first coordinate...
    LineToRotate.first  = RotatePointAboutAnother(LineToRotate.first, Origin, fRadians);
    
    // Apply rotation about the specified origin for the second coordinate...
    LineToRotate.second = RotatePointAboutAnother(LineToRotate.second, Origin, fRadians);
}

// Rotate a point around another to be used as the origin...
inline CvPoint &Worm::RotatePointAboutAnother(CvPoint const &OldPointToRotate, CvPoint const &Origin, 
                                              float const &fRadians, CvPoint &NewPoint) const
{
    /* This is the decomposed form of the combined linear transformation that translates back to origin, rotates 
       about the origin, then translates back again to the starting point, built from this transformation...
      
            | 1  0  r_x |     | cos(θ)  -sin(θ)   0 |     | 1  0 -r_x |   | x |
            | 0  1  r_y |  *  | sin(θ)   cos(θ)   0 |  *  | 0  1 -r_y | * | y |
            | 0  0  1   |     |    0        0     1 |     | 0  0   1  |   | 1 |
            
                (3)                    (2)                    (1)
            
            (1) First translate coordinate system back to real origin.
            (2) Rotate about the real origin.
            (3) Restore coordinate system back.
            
            Note: Transforms are applied in reverse order, like a stack.
    */
    
    
        // Calculate new x-coordinate... 
        NewPoint.x = (cos(fRadians) * OldPointToRotate.x) -
                     (sin(fRadians) * OldPointToRotate.y) +
                     (Origin.x * (1 - cos(fRadians))) +
                     (Origin.y * sin(fRadians));
       
        // Calculate new y-coordinate...
        NewPoint.y = (sin(fRadians) * OldPointToRotate.x) +
                     (cos(fRadians) * OldPointToRotate.y) +
                     (Origin.y * (1 - cos(fRadians))) -
                     (Origin.x * sin(fRadians));

    // Done...
    return &NewPoint;                
}

// Best guess as to the tail's position at this moment in time, since it changes...
CvPoint Worm::Tail() const
{
    /* TODO: Implement this. */
}

// Update the approximate area, based on the value at this moment in time. This will help us make a
//  more informed answer when asked via Area() for the size. θ(1) space and time...
inline void &Worm::UpdateArea(float const &fAreaAtThisMoment)
{
    // Store the new arithmetic mean in constant space. Just multiply your old average by n, 
    //  add x_{n+1}, and then divide the whole thing by n+1...
    fArea = (((fArea * unUpdates) + fAreaAtThisMoment) / (unUpdates + 1);
}

// Update the approximate head and tail position, based on the value at this moment in time. This will
//  help us make a more informed answer when asked via Head() or Tail() for the actual coordinates.
//  θ(1) space and time...
inline void &Worm::UpdateHeadAndTail(unsigned int const &unHeadVertexIndex,
                                     unsigned int const &unTailVertexIndex)
{
    /* TODO: Implement this. */
}

// Update the approximate length, based on the value at this moment in time. This will help us make a
//  more informed answer when asked via Length() for the length. θ(1) space and time...
inline void &Worm::UpdateLength(float const &fLengthAtThisMoment)
{
    // Store the new arithmetic mean in constant space. Just multiply your old average by n, 
    //  add x_{n+1}, and then divide the whole thing by n+1...
    fLength = (((fLength * unUpdates) + fLengthAtThisMoment) / (unUpdates + 1);
}

// Update the approximate width, based on the value at this moment in time. This will help us make a
//  more informed answer when asked via Width() for the width. θ(1) space and time...
inline void &Worm::UpdateWidth(float const &fWidthAtThisMoment)
{
    // Store the new arithmetic mean in constant space. Just multiply your old average by n, 
    //  add x_{n+1}, and then divide the whole thing by n+1...
    fWidth = (((fWidth * unUpdates) + fWidthAtThisMoment) / (unUpdates + 1);
}

// Best guess of the area, considering everything we've seen thus far...
inline float const &Worm::Width() const
{
    // Return it...
    return fWidth;
}
    
// Deconstructor...
Worm::~Worm()
{
    // Deallocate storage pool...
    cvReleaseMemStorage(&pStorage);
}
