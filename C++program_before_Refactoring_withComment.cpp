// Function to fit and adjust cut lines between graphic objects, handling special cases and intersections
BOOL Try::fitCutLine(MSGraphicList* list, int type, MSMPointMap* checkCrossPointMap, MPoint moveOffset)
{
    // Initialize basic graphic variables for manipulation
    MSGraphic* aGraphic;
    MSGraphicID	gID;
    MPoint		sP;
    MPoint		eP;

    // Special case: Handle single item in list
    if (list->GetCount() == 1)
    {
        aGraphic = list->GetHead();
        gID = aGraphic->getGraphicID();
        ((MSPath*)aGraphic)->getStartEnd(&sP, &eP);
        if (type == MS_SPECIAL1)
            checkCrossPointMap->SetAt(gID.serialNo, sP);  // Store start point for special case
        else
            checkCrossPointMap->SetAt(gID.serialNo, eP);  // Store end point for normal case
        return TRUE;
    }

    // Handle start/end point storage based on type
    if (type == MS_SPECIAL1)
    {
        // Store start point of first graphic for special case
        aGraphic = list->GetHead();
        gID = aGraphic->getGraphicID();
        ((MSPath*)aGraphic)->getStartEnd(&sP, &eP);
        checkCrossPointMap->SetAt(gID.serialNo, sP);
    }
    else
    {
        // Store end point of last graphic for normal case
        aGraphic = list->GetTail();
        gID = aGraphic->getGraphicID();
        ((MSPath*)aGraphic)->getStartEnd(&sP, &eP);
        checkCrossPointMap->SetAt(gID.serialNo, eP);
    }

    // Calculate canvas boundaries with offset
    MRect		canvasRect;
    getCanvasRect(&canvasRect);
    MPoint		minP;
    MPoint		maxP;
    minP.x = canvasRect.origin.x + moveOffset.x;
    minP.y = canvasRect.origin.y + moveOffset.y;
    maxP.x = canvasRect.origin.x + moveOffset.x + canvasRect.size.width;
    maxP.y = canvasRect.origin.y + moveOffset.y + canvasRect.size.height;

    // Initialize variables for graphic processing
    MSGraphic* obj1;
    MSGraphic* obj2;
    POSITION		nPos = NULL;
    POSITION		ePos = NULL;
    MPoint			sP1;
    MPoint			sP2;
    MPoint			eP1;
    MPoint			eP2;
    MPoint			midP;

    // Lists for managing graphics operations
    MSGraphicList	deleteList;
    MSGraphicList	fList;
    MSGraphicList	resultList;
    int				ret;

    // Variables for distance calculations
    MPoint			checkP;
    float			minDist = 99999.0f;
    float			maxDist = -1.0f;
    float			dist;

    // Variables for handling intersections
    MSGraphicList	crossList;
    MSGraphic* crossObj;
    FMCrossPoint	mfCrossP;
    FMCrossPoint	targetMfP;
    POSITION		dPos = NULL;
    unsigned int	serialNo;
    BOOL			findFlag = FALSE;

    // Lists for categorizing intersection points
    MSGraphicList	fullInnerList;
    MSGraphicList	normalList;
    MSGraphicList	abnormalList;

    // Main processing loop - iterate through graphics list
    for (nPos = list->GetHeadPosition(); nPos != NULL; )
    {
        // Get current and next graphic objects
        if (nPos == list->GetHeadPosition())
            obj1 = list->GetAt(nPos);
        ePos = nPos;
        list->GetNext(ePos);
        if (ePos == NULL)
            break;
        obj2 = list->GetAt(ePos);

        // Calculate midpoint between end of first object and start of second
        ((MSPath*)obj1)->getStartEnd(&sP1, &eP1);
        ((MSPath*)obj2)->getStartEnd(&sP2, &eP2);
        midP.x = (eP1.x + sP2.x) / 2.0f;
        midP.y = (eP1.y + sP2.y) / 2.0f;

        // Check distance between endpoints
        this -> GetPointDistance(&eP1, &sP2, &dist);
        findFlag = FALSE;

        // Process intersections if points are not too close
        if (dist >= DEF_SAME_DIST)
        {
            minDist = 99999.0f;
            
            // Find intersection points between objects
            ret = this -> getCrossPoint(obj1, obj2, &crossList);

            // Process found intersection points
            if ((ret == MS_NORMAL) && (crossList.GetCount()))
            {
                // Categorize intersection points based on their position
                for (dPos = crossList.GetHeadPosition(); dPos != NULL;
                    crossList.GetNext(dPos))
                {
                    crossObj = crossList.GetAt(dPos);
                    mfCrossP = ((MSCrossPoint*)crossObj)->getCrossPoint();

                    // Skip points outside canvas boundaries
                    if ((mfCrossP.point.x < minP.x) ||
                        (mfCrossP.point.x > maxP.x) ||
                        (mfCrossP.point.y < minP.y) ||
                        (mfCrossP.point.y > maxP.y))
                        continue;

                    // Categorize intersection points based on their status
                    switch (mfCrossP.status)
                    {
                        case MF_FULLINNER:
                            fullInnerList.AddTail(crossObj);
                            break;
                        case FM_LEFTOUTER:
                            if (mfCrossP.side[0] == FM_ENDPOINT)
                                normalList.AddTail(crossObj);
                            else
                                abnormalList.AddTail(crossObj);
                            break;
                        case FM_RIGHTOUTER:
                            if (mfCrossP.side[1] == FM_STARTPOINT)
                                normalList.AddTail(crossObj);
                            else
                                abnormalList.AddTail(crossObj);
                            break;
                        case FM_FULLOUTER:
                            if ((mfCrossP.side[0] == FM_ENDPOINT) &&
                                (mfCrossP.side[1] == FM_STARTPOINT))
                                normalList.AddTail(crossObj);
                            else
                                abnormalList.AddTail(crossObj);
                            break;
                    }
                }

                // Process full inner intersections
                if (fullInnerList.GetCount())
                {
                    // Find furthest intersection point
                    maxDist = -1.0f;
                    checkP = eP2;

                    for (dPos = fullInnerList.GetHeadPosition(); dPos != NULL;
                        fullInnerList.GetNext(dPos))
                    {
                        crossObj = fullInnerList.GetAt(dPos);
                        mfCrossP = ((MSCrossPoint*)crossObj)->getCrossPoint();
                        this => GetPointDistance(&(mfCrossP.point), &checkP, &dist);
                        if (dist > maxDist)
                        {
                            maxDist = dist;
                            targetMfP = mfCrossP;
                            findFlag = TRUE;
                        }
                    }
                }
                // Process normal intersections if no full inner intersections
                else if (normalList.GetCount())
                {
                    // Find nearest intersection point
                    minDist = 99999.0f;
                    checkP = sP2;

                    for (dPos = normalList.GetHeadPosition(); dPos != NULL;
                        normalList.GetNext(dPos))
                    {
                        crossObj = normalList.GetAt(dPos);
                        mfCrossP = ((MSCrossPoint*)crossObj)->getCrossPoint();
                        this => GetPointDistance(&(mfCrossP.point), &checkP, &dist);
                        if (dist < minDist)
                        {
                            minDist = dist;
                            targetMfP = mfCrossP;
                            findFlag = TRUE;
                        }
                    }
                }
            }

            // Clean up intersection lists
            if (fullInnerList.GetCount())
                fullInnerList.RemoveAll();
            if (normalList.GetCount())
                normalList.RemoveAll();
            if (abnormalList.GetCount())
                abnormalList.RemoveAll();
            if (crossList.GetCount())
                crossList.FreeAll();
        }

        // Update cross point map with intersection or midpoint
        serialNo = obj1->getSerialNo();
        if (type == MS_SPECIAL1)
            serialNo++;

        if (findFlag == TRUE)
            checkCrossPointMap->SetAt(serialNo, targetMfP.point);
        else
            checkCrossPointMap->SetAt(serialNo, midP);

        // Fit paths between objects
        ret = this ->getFitPathAtDirect(obj1, HM_END, obj2, MS_START, &fList, FALSE);
        if (ret == MS_ERROR)
        {
            if (deleteList.GetCount())
                deleteList.FreeAll();
            return FALSE;
        }

        // Manage graphics lists and update endpoints
        if (nPos != list->GetHeadPosition())
            deleteList.AddTail(obj1);

        obj1 = fList.GetHead();
        obj2 = fList.GetTail();

        ((MSPath*)obj1)->setEnd(midP);
        ((MSPath*)obj2)->setStart(midP);

        fList.RemoveAll();
        resultList.AddTail(obj1);
        obj1 = obj2;

        nPos = ePos;
    }

    // Finalize result list
    resultList.AddTail(obj1);

    // Clean up temporary lists
    if (deleteList.GetCount())
        deleteList.FreeAll();

    if (list->GetCount())
        list->FreeAll();

    // Update original list with results
    list->AddTail(&resultList);

    return TRUE;
}