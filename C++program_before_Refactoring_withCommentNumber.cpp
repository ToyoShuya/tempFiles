// [1] Function to fit a cut line between graphic objects in a list
BOOL Try::fitCutLine(MSGraphicList* list, int type, MSMPointMap* checkCrossPointMap, MPoint moveOffset)
{
	MSGraphic* aGraphic;
	MSGraphicID	gID;
	MPoint		sP;
	MPoint		eP;

	// [2] Special handling for single item list - store either start or end point
	if (list->GetCount() == 1)
	{
		aGraphic = list->GetHead();
		gID = aGraphic->getGraphicID();
		((MSPath*)aGraphic)->getStartEnd(&sP, &eP);
		if (type == MS_SPECIAL1)
			checkCrossPointMap->SetAt(gID.serialNo, sP);
		else
			checkCrossPointMap->SetAt(gID.serialNo, eP);
		return TRUE;
	}

	// [3] For multiple items, store start point of first or end point of last based on type
	if (type == MS_SPECIAL1)
	{
		aGraphic = list->GetHead();
		gID = aGraphic->getGraphicID();
		((MSPath*)aGraphic)->getStartEnd(&sP, &eP);
		checkCrossPointMap->SetAt(gID.serialNo, sP);
	}
	else
	{
		aGraphic = list->GetTail();
		gID = aGraphic->getGraphicID();
		((MSPath*)aGraphic)->getStartEnd(&sP, &eP);
		checkCrossPointMap->SetAt(gID.serialNo, eP);
	}

	// [4] Calculate canvas boundaries with offset
	MRect		canvasRect;
	getCanvasRect(&canvasRect);
	MPoint		minP;
	MPoint		maxP;
	minP.x = canvasRect.origin.x + moveOffset.x;
	minP.y = canvasRect.origin.y + moveOffset.y;
	maxP.x = canvasRect.origin.x + moveOffset.x + canvasRect.size.width;
	maxP.y = canvasRect.origin.y + moveOffset.y + canvasRect.size.height;

	// [5] Initialize variables for processing graphics
	MSGraphic* obj1;
	MSGraphic* obj2;
	POSITION		nPos = NULL;
	POSITION		ePos = NULL;
	MPoint			sP1;
	MPoint			sP2;
	MPoint			eP1;
	MPoint			eP2;
	MPoint			midP;

	// [6] Initialize lists for managing graphics
	MSGraphicList	deleteList;
	MSGraphicList	fList;
	MSGraphicList	resultList;
	int				ret;

	// [7] Variables for distance calculations
	MPoint			checkP;
	float			minDist = 99999.0f;
	float			maxDist = -1.0f;
	float			dist;

	// [8] Initialize variables for intersection checking
	MSGraphicList	crossList;
	MSGraphic* crossObj;
	FMCrossPoint	mfCrossP;
	FMCrossPoint	targetMfP;
	POSITION		dPos = NULL;
	unsigned int	serialNo;
	BOOL			findFlag = FALSE;

	// [9] Lists for categorizing intersection points
	MSGraphicList	fullInnerList;
	MSGraphicList	normalList;
	MSGraphicList	abnormalList;

	// [10] Main loop: Process each pair of consecutive graphics
	for (nPos = list->GetHeadPosition(); nPos != NULL; )
	{
		// [11] Get current and next graphic objects
		if (nPos == list->GetHeadPosition())
			obj1 = list->GetAt(nPos);
		ePos = nPos;
		list->GetNext(ePos);
		if (ePos == NULL)
			break;
		obj2 = list->GetAt(ePos);

		// [12] Calculate endpoints and midpoint between graphics
		((MSPath*)obj1)->getStartEnd(&sP1, &eP1);
		((MSPath*)obj2)->getStartEnd(&sP2, &eP2);
		midP.x = (eP1.x + sP2.x) / 2.0f;
		midP.y = (eP1.y + sP2.y) / 2.0f;

		// [13] Check distance between endpoints
		this -> GetPointDistance(&eP1, &sP2, &dist);
		findFlag = FALSE;

		// [14] If distance exceeds threshold, find intersection points
		if (dist >= DEF_SAME_DIST)
		{
			minDist = 99999.0f;
			ret = this -> getCrossPoint(obj1, obj2, &crossList);

			// [15] Process intersection points if found
			if ((ret == MS_NORMAL) && (crossList.GetCount()))
			{
				// [16] Categorize each intersection point
				for (dPos = crossList.GetHeadPosition(); dPos != NULL;
					crossList.GetNext(dPos))
				{
					crossObj = crossList.GetAt(dPos);
					mfCrossP = ((MSCrossPoint*)crossObj)->getCrossPoint();

					// [17] Skip points outside canvas boundaries
					if ((mfCrossP.point.x < minP.x) ||
						(mfCrossP.point.x > maxP.x) ||
						(mfCrossP.point.y < minP.y) ||
						(mfCrossP.point.y > maxP.y))
						continue;

					// [18] Categorize intersection point based on status
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

				// [19] Process full inner intersections
				if (fullInnerList.GetCount())
				{
					maxDist = -1.0f;
					checkP = eP2;

					// [20] Find furthest intersection point
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
				// [21] Process normal intersections if no full inner intersections
				else if (normalList.GetCount())
				{
					minDist = 99999.0f;
					checkP = sP2;

					// [22] Find nearest intersection point
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

			// [23] Cleanup temporary lists
			if (fullInnerList.GetCount())
				fullInnerList.RemoveAll();
			if (normalList.GetCount())
				normalList.RemoveAll();
			if (abnormalList.GetCount())
				abnormalList.RemoveAll();
			if (crossList.GetCount())
				crossList.FreeAll();
		}

		// [24] Update cross point map with intersection or midpoint
		serialNo = obj1->getSerialNo();
		if (type == MS_SPECIAL1)
			serialNo++;

		if (findFlag == TRUE)
			checkCrossPointMap->SetAt(serialNo, targetMfP.point);
		else
			checkCrossPointMap->SetAt(serialNo, midP);

		// [25] Fit paths between objects
		ret = this ->getFitPathAtDirect(obj1, HM_END, obj2, MS_START, &fList, FALSE);
		if (ret == MS_ERROR)
		{
			if (deleteList.GetCount())
				deleteList.FreeAll();
			return FALSE;
		}

		// [26] Manage graphics lists and update endpoints
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

	// [27] Finalize result list
	resultList.AddTail(obj1);

	// [28] Cleanup and update original list
	if (deleteList.GetCount())
		deleteList.FreeAll();

	if (list->GetCount())
		list->FreeAll();

	list->AddTail(&resultList);

	return TRUE;
}