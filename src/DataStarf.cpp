//===========================================================================
// DataStarf.cpp
//
// CDataStarf
//   starfield data.
//   a starfield contains stars, constellations, information about time and
//   location on Earth, as well as certain options. A CDataStarf and 
//   most attributes of a CDataStarf can be saved and opened.
//===========================================================================


#include "stdafx.h"
#include "CSApp.h"
#include "DataStarf.h"

#include "DataStar.h"
#include "DataConst.h"

IMPLEMENT_SERIAL( CDataStarf, CObject, 1 )


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CDataStarf::CDataStarf()
{
	Clear();
}

CDataStarf::~CDataStarf()
{
}

// Clear starfield. Also clear stars if told to do so
void CDataStarf::Clear( BOOL clearStars )
{
	// Stars
	if( clearStars )
	{
		seed = 0;
		starCount = 0;
		stars.clear();
		stars.push_back( CDataStar() );
	}

	// Constellations
	constCount = 0;
	newConstCount = 0;
	curConstNum = 0;
	consts.clear();

	// Viewing
	viewMat.identity();
	timeMat.identity();
	latMat.identity();

	rotX = 0.0f;
	rotY = 0.0f;
	rotTime = 0.0f;
	zoom = 0.0f;

	spinning = FALSE;

	tracking = FALSE;
	trackX = 0.0f;
	trackY = 0.0f;
	trackZ = 0.0f;

	// Defaults
	LoadStarDefaults();
	LoadConstDefaults();
	LoadSunDefaults();

	// Location Time
	time_t seconds = time(NULL);
	gregorian = *(localtime( &seconds ));
	gregorian.tm_year += 1900;
	latitude = 34.6f;//34.6N for Portales
	longitude = 0.0f;
}

// Create a new random or actual starfield
void CDataStarf::New( BOOL actual )
{
	// Don't clear stars if we don't need to
	if( actual && seed == -1 )
		Clear( FALSE );
	else
		Clear();

	if( actual )
	{
		// Check if we've already loaded actual stars
		if( seed != -1 )
		{
			seed = -1;
			InitActualStars();
		}
		InitActualConsts();
		UpdateMats();
	}
	else
	{
		seed = clock();
		InitRandomStars();
		UpdateMats();
	}
}

// Creates sphere of random stars with radius of 1
void CDataStarf::InitRandomStars()
{
	// Seed random numbers for predictable random starfield
	srand( (unsigned)seed );

	starCount = MAX_STARS;
	CDataStar newStar;

	/* /// North Star
	newStar.SetColor( COLOR_GREEN );
	newStar.SetMag( 2.0f );
	newStar.SetRA( 0, 0, 0.0f );
	newStar.SetDec( TRUE, 90, 0, 0.0f );
	newStar.SetCenter( vector3(0.0f,1.0f,0.0f) );
	stars.push_back( CDataStar(newStar) );
	*/

	// Sun Corona
	newStar.SetColor( COLOR_WHITE );
	newStar.SetMag( -10.0f );
	newStar.SetRA( 12, 0, 0.0f );
	newStar.SetDec( TRUE, 0, 0, 0.0f );
	newStar.SetCenter( vector3(0.0f,0.0f,-1.0f) );
	stars.push_back( CDataStar(newStar) );

	// Randomize the rest
	for (int i=1; i<starCount; ++i)
	{
		newStar.Randomize();
		stars.push_back( CDataStar(newStar) );
	}

	// Sort stars by magnitude
	std::sort( stars.begin(), stars.end() );

	CountStars();
	starfMgr.UpdateStarsAppearance();
}

// Load the actual stars from the magnitude-sorted hipparcos catalog
void CDataStarf::InitActualStars()
{
	CDataStar newStar;

	CFile file;
	if( !file.Open( "data\\hip_main.txt", CFile::modeRead ) )
	{
		CSDebug( "Unable to open star catalog", "CDataStarf::InitActualStars" );
		exit(0);
		return;
	}

	char buffer[100];
	ra_s ra = {0};
	dec_s dec = {0};
	float mag = 0;

	// Initially load all stars
	for( int i=0; i<MAX_STARS; ++i )
	{
		file.Seek( 17, CFile::current );
		ZeroMemory(&buffer, sizeof(buffer));

		// Right Ascension
		file.Read( buffer, 2 );
		ra.hour = atoi( buffer );
			file.Seek( 1, CFile::current );
			ZeroMemory(&buffer, sizeof(buffer));
		file.Read( buffer, 2 );
		ra.minute = atoi( buffer );
			file.Seek( 1, CFile::current );
			ZeroMemory(&buffer, sizeof(buffer));
		file.Read( buffer, 5 );
		ra.second = (float) atof( buffer );

		file.Seek( 1, CFile::current );
		ZeroMemory(&buffer, sizeof(buffer));

		// Declination
		file.Read( buffer, 1 );
		dec.positive = buffer[0] == '+';

		file.Read( buffer, 2 );
		dec.degree = atoi( buffer );
			file.Seek( 1, CFile::current );
			ZeroMemory(&buffer, sizeof(buffer));
		file.Read( buffer, 2 );
		dec.minute = atoi( buffer );
			file.Seek( 1, CFile::current );
			ZeroMemory(&buffer, sizeof(buffer));
		file.Read( buffer, 4 );
		dec.second = (float) atof( buffer);

		file.Seek( 1, CFile::current );
		ZeroMemory(&buffer, sizeof(buffer));

		// Magnitude
		file.Read( buffer, 5 );
		mag = (float) atof( buffer );

		// Ignore rest of line
		file.Seek( 406, CFile::current );

		newStar.SetRA( ra );
		newStar.SetDec( dec );
		newStar.SetMag( mag );
		newStar.UpdatePosFromRADec();

		stars.push_back( CDataStar(newStar) );
	}

	CountStars();
	starfMgr.UpdateStarsAppearance();

	file.Close();
}

/// Load actual constellations somehow
void CDataStarf::InitActualConsts()
{
}


/////////////////////////////////////////////////////////////////////////////
// Gets

CDataStar*	CDataStarf::GetStar(int i)			{	return &stars[i];				}
int			CDataStarf::GetStarCount()			{	return starCount;				}
CDataConst*	CDataStarf::GetConst(int i)			{	return &consts[i];				}
CDataConst*	CDataStarf::GetCurConst()			{	return &consts[curConstNum];	}
int			CDataStarf::GetConstCount()			{	return constCount;				}
int			CDataStarf::GetCurConstNum()		{	return curConstNum;				}
int			CDataStarf::GetNewConstCount()		{	return newConstCount;			}
BOOL		CDataStarf::AreStarsVisible()		{	return starsVisible;			}
BOOL		CDataStarf::AreStarsLabeled()		{	return starsLabeled;			}
BOOL		CDataStarf::AreConstsVisible()		{	return constsVisible;			}
BOOL		CDataStarf::AreConstsLabeled()		{	return constsLabeled;			}
BOOL		CDataStarf::IsSunVisible()			{	return sunVisible;				}
BOOL		CDataStarf::IsSunShining()			{	return sunShine;				}
matrix44*	CDataStarf::GetViewMat()			{	return &viewMat;				}
matrix44*	CDataStarf::GetTimeMat()			{	return &timeMat;				}
matrix44*	CDataStarf::GetLatMat()				{	return &latMat;					}
float		CDataStarf::GetRotX()				{	return rotX;					}
float		CDataStarf::GetRotY()				{	return rotY;					}
float		CDataStarf::GetRotTime()			{	return rotTime;					}
float		CDataStarf::GetZoom()				{	return zoom;					}
BOOL		CDataStarf::IsSpinning()			{	return spinning;				}
BOOL		CDataStarf::IsTracking()			{	return tracking;				}

// Find the constellation with the given name
CDataConst*	CDataStarf::GetConst( CString& name )
{
	for( int i=0; i<constCount; ++i )
	{
		if( GetConst(i)->GetName() == name )
			return &consts[i];
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Sets

void CDataStarf::IncNewConstCount()				{	newConstCount++;				}
void CDataStarf::SwitchStarsVisible()			{	starsVisible = !starsVisible;	}
void CDataStarf::SetStarsVisible( BOOL x )		{	starsVisible = x;				}
void CDataStarf::SwitchStarsLabeled()			{	starsLabeled = !starsLabeled;	}
void CDataStarf::SetStarsLabeled( BOOL x )		{	starsLabeled = x;				}
void CDataStarf::SwitchConstsVisible()			{	constsVisible = !constsVisible;	}
void CDataStarf::SetConstsVisible( BOOL x )		{	constsVisible = x;				}
void CDataStarf::SwitchConstsLabeled()			{	constsLabeled = !constsLabeled;	}
void CDataStarf::SetConstsLabeled( BOOL x )		{	constsLabeled = x;				}
void CDataStarf::SwitchSunVisible()				{	sunVisible = !sunVisible;		}
void CDataStarf::SetSunVisible( BOOL x )		{	sunVisible = x;					}
void CDataStarf::SwitchSunShine()				{	sunShine = !sunShine;			}
void CDataStarf::SetSunShine( BOOL x )			{	sunShine = x;					}
void CDataStarf::SwitchSpinning()				{	spinning = !spinning;			}

void CDataStarf::SetRotTime( float r, BOOL updateMat )
{
	rotTime = r;
	if( updateMat )
		UpdateTimeMat();
}

void CDataStarf::AdjRotX( float delta, BOOL updateMat )
{	
	// Restrict up and down rotation
	float newRotX = rotX + delta;

	if( newRotX > -90 && newRotX < 90 )
		rotX = newRotX;

	if( updateMat )
		UpdateViewMat();
}

void CDataStarf::AdjRotY( float delta, BOOL updateMat )
{
	rotY += delta;

	// Keep rotY between -360 and 360
	if( rotY < -360.0f )
		rotY += 360.0f;
	if( rotY > 360.0f )
		rotY -= 360.0f;

	if( updateMat )
		UpdateViewMat();
}

void CDataStarf::AdjRotTime( float delta, BOOL updateMat )
{
	rotTime += delta;
	if( tracking )
		Track();
	if( updateMat )
		UpdateTimeMat();
}

void CDataStarf::AdjZoom( float delta )
{
	zoom += delta;
}


/////////////////////////////////////////////////////////////////////////////
// Methods


/////////////////////////////////////////////////////////////////////////////
// Star Methods

// Load default star options
void CDataStarf::LoadStarDefaults()
{
	starsVisible = DEF_STARS_VISIBLE;
	starsLabeled = DEF_STARS_LABELED;
}

// Count the number of stars brighter than the limiting magnitude
void CDataStarf::CountStars()
{
	starCount = 0;

	for( int i=0; i<MAX_STARS; ++i )
	{
		if( GetStar(i)->GetMag() <= optionsMgr.GetStarsLimMag() )
			++starCount;
		else
			break;
	}
}

// Check if the given star number belongs to a hidden constellation
BOOL CDataStarf::IsStarInHiddenConst( int i )
{
	for( int ci=0; ci<constCount; ++ci )
	{
		// Continue if this constellation isn't hidden
		if( GetConst(ci)->IsVisible() )
			continue;
		// Otherwise search through lines
		for( int li=0; li<GetConst(ci)->GetLineCount(); ++li )
		{
			if( i == GetConst(ci)->GetLine(li)->GetStar1() ||
				i == GetConst(ci)->GetLine(li)->GetStar2() )
				return TRUE;
		}
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Constellation Methods

// Load default constellation options
void CDataStarf::LoadConstDefaults()
{
	constsVisible = DEF_CONST_VISIBLE;
	constsLabeled = DEF_CONST_LABELED;
}

// Check if the given name is already in use
BOOL CDataStarf::IsDuplicate( CString& name )
{
	for( int i=0; i<constCount; ++i )
	{
		if( GetConst(i)->GetName() == name )
			return TRUE;
	}

	return FALSE;
}

// Add a new constellation with the given name
void CDataStarf::AddConst( CString& name )
{
	consts.push_back( CDataConst(name) );
	++constCount;
}

// Delete the current constellation
void CDataStarf::DeleteConst()
{
	// Use an iterator to find the current constellation
	const_vi constsItor = consts.begin();
	for( int i=0; i<curConstNum; ++i )
		++constsItor;

	// Delete the constellation
	consts.erase( constsItor );

	--constCount;
}

// Rename the current constellation to the given name
void CDataStarf::RenameConst( CString& name )
{
	GetCurConst()->SetName(name);
}

// Set the current constellation to the one with the given name
BOOL CDataStarf::SelectConst( CString& name )
{
	// Search for constellation name
	for( int i=0; i<constCount; ++i )
	{
		if( GetConst(i)->GetName() == name )
		{
			curConstNum = i;
			return TRUE;
		}
	}

	// Return FALSE if name wasn't found
	return FALSE;
}

// Set the current constellation to the one with the given number
void CDataStarf::SelectConst( int i )
{
	curConstNum = i;
}


/////////////////////////////////////////////////////////////////////////////
// Sun methods

// Load default sun options
void CDataStarf::LoadSunDefaults()
{
	sunVisible = DEF_SUN_VISIBLE;
	sunShine = DEF_SUN_SHINE;
}


/////////////////////////////////////////////////////////////////////////////
// View Methods

// Update all matrices
void CDataStarf::UpdateMats()
{
	UpdateViewMat();
	UpdateTimeMat();
	UpdateLatMat();
	UpdateLongMat();
}
// Update view matrix
void CDataStarf::UpdateViewMat()
{
	viewMat.identity();
	viewMat = RotateRadMatrix44( 'x', DegToRad( rotX ) );
	viewMat *= RotateRadMatrix44( 'y', DegToRad( rotY ) );
}

// Update time matrix
void CDataStarf::UpdateTimeMat()
{
	timeMat.identity();
	timeMat = RotateRadMatrix44( 'y', DegToRad( rotTime ) );
}

// Update latitude matrix
void CDataStarf::UpdateLatMat()
{
	latMat.identity();
	latMat = RotateRadMatrix44( 'x', DegToRad( latitude-90.0f ) );
}

// Update longitude matrix
void CDataStarf::UpdateLongMat()
{
	longMat.identity();
	longMat = RotateRadMatrix44( 'y', 0.0f );///
}

// Rotate methods (for keyboard)
void CDataStarf::RotateUp()		{	AdjRotX(-0.5f);	}
void CDataStarf::RotateDown()	{	AdjRotX( 0.5f);	}
void CDataStarf::RotateLeft()	{	AdjRotY(-0.5f);	}
void CDataStarf::RotateRight()	{	AdjRotY( 0.5f);	}

// Zoom methods (for keyboard and mousewheel)
void CDataStarf::ZoomIn()
{
	if( zoom < 0.9f )
		zoom += 0.02f*(1-zoom);
}
void CDataStarf::ZoomOut()
{
	if( zoom > -0.8f )
		zoom -= 0.02f*(1-zoom);
}

// Reset viewing rotation and zoom
void CDataStarf::ResetView()
{
	ResetRot();
	ResetZoom();
}

// Reset rotation
void CDataStarf::ResetRot()
{
	rotX = 0.0f;
	rotY = 0.0f;
	rotTime = 0.0f;
	UpdateViewMat();
	UpdateTimeMat();
}

// Reset zoom
void CDataStarf::ResetZoom()
{
	zoom = 0.0f;
}

// Stop tracking
void CDataStarf::StopTracking()
{
	tracking = FALSE;
}

// Start tracking the specified spherical coordinates
void CDataStarf::StartTracking( float x, float y, float z )
{
	tracking = TRUE;
	trackX = x;
	trackY = y;
	trackZ = z;
	Track();
}

// Track the spherical coordinates set in StartTracking
void CDataStarf::Track()
{
//  Coordinates specify a point on celectial sphere
//  (ie. doesn't include latitude or time rotation)

	// Find world coordinates (ie. after latitude and time rotation)
	vector4 worldVec( trackX, trackY, trackZ, 1.0f );
	worldVec = latMat * timeMat * worldVec;

	// Make up a fake star so we can use its methods
	//   (little sloppy, but it'll work)
	CDataStar fakeStar;
	fakeStar.SetCenter( vector3(worldVec.x, worldVec.y, worldVec.z) );
	fakeStar.UpdatePosFromXYZ();

	rotX = fakeStar.GetPhi() - 90.0f;
	rotY = 180.0f - fakeStar.GetTheta();

	UpdateViewMat();
}

// Find and view the specified spherical coordinates
void CDataStarf::Find( float x, float y, float z )
{
//  Coordinates specify a point on celectial sphere
//  (ie. doesn't include latitude or time rotation)

	// Find world coordinates (ie. after latitude and time rotation)
	vector4 worldVec( x, y, z, 1.0f );
	worldVec = latMat * timeMat * worldVec;

	// Make up a fake star so we can use its methods
	//   (little sloppy, but it'll work)
	CDataStar fakeStar;
	fakeStar.SetCenter( vector3(worldVec.x, worldVec.y, worldVec.z) );
	fakeStar.UpdatePosFromXYZ();

	rotX = fakeStar.GetPhi() - 90.0f;
	rotY = 180.0f - fakeStar.GetTheta();

	UpdateViewMat();
}


/////////////////////////////////////////////////////////////////////////////
// Serialization

void CDataStarf::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);
	int i;

	// Serialize CDataStarf attributes
	if( ar.IsLoading() )
	{
		// Prevent loading actual stars over again
		BOOL isActual = seed == -1;

		ar >> seed
		   >> constCount >> newConstCount >> curConstNum
		   >> latitude >> longitude
		   >> rotX >> rotY >> rotTime
		   >> zoom;

		// If we're loading an actual starfield
		if( seed == -1 )
		{
			// Only InitActualStars if we haven't already
			if( !isActual )
			{
				stars.clear();
				InitActualStars();
			}
			UpdateMats();
		}
		// Otherwise initialize random starfield
		else
		{
			stars.clear();
			InitRandomStars();
			UpdateMats();
		}
	}
	else
	{
		ar << seed
		   << constCount << newConstCount << curConstNum
		   << latitude << longitude
		   << rotX << rotY << rotTime
		   << zoom;
	}

	// If we're loading, get the constellations vector ready
	if( ar.IsLoading() )
	{
		consts.clear();
		for( i=0; i<constCount; ++i )
			consts.push_back( CDataConst() );
	}

	// Serialize constellations
	for( i=0; i<constCount; ++i )
		GetConst(i)->Serialize( ar );
}



