#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/DisplayList.h"
#include "cinder/gl/Material.h"
#include "cinder/ImageIo.h"

#include "cinder/params/Params.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static const Vec3f	CAM_POSITION_INIT( 0.0f, 0.0f, -8.0f);
static const Vec3f	LIGHT_POSITION_INIT( 0.0f, 4.0f, 0.0f );

enum
{
	SHOW_STANDARD_VIEW,
	SHOW_SSAO,
	SHOW_NORMALMAP,
	SHOW_FINAL_SCENE
};

//being lazy and keeping not putting header in another file
class Base_ThreeD_ProjectApp : public AppBasic 
{
	public:
		Base_ThreeD_ProjectApp();
		virtual ~Base_ThreeD_ProjectApp();
		void prepareSettings( Settings *settings );
		
		void setup();
		void update();
		void draw();
		
		void mouseDown( MouseEvent event );	
		void keyDown( app::KeyEvent event ); 
	
        //render functions ( when looking to optimize GPU look here ... )
		void drawTestObjects();
		void renderSceneToFBO();
		void renderNormalsDepthToFBO();
		void renderSSAOToFBO();	
		void pingPongBlur();	
        void renderScreenSpace();
        
        void initShaders();
        void initFBOs();
    
	protected:
	
		int RENDER_MODE;
	
		//debug
		cinder::params::InterfaceGl mParams;
		bool				mShowParams;
		float				mCurrFramerate;
		bool				mLightingOn;
		bool				mViewFromLight;
	
		//objects
		gl::DisplayList		mTorus, mBoard, mBox, mSphere;
	
		//camera
		CameraPersp			*mCam;
		Vec3f				mEye;
		Vec3f				mCenter;
		Vec3f				mUp;
		float				mCameraDistance;
	
		//light
		gl::Light			*mLight;
		gl::Light			*mLightRef;
	
		gl::Fbo				mScreenSpace1;
		gl::Fbo				mNormalDepthMap;
		gl::Fbo				mSSAOMap;
		gl::Fbo				mFinalScreenTex;
	
		gl::Fbo				mPingPongBlurH;
		gl::Fbo				mPingPongBlurV;
	
		gl::Texture			mRandomNoise;
	
		gl::GlslProg		mSSAOShader;
		gl::GlslProg		mNormalDepthShader;
		gl::GlslProg		mBasicBlender;
		gl::GlslProg		mHBlurShader;
		gl::GlslProg		mVBlurShader;
};

/* 
 * @Description: constructor
 * @param: none
 * @return: none
 */
Base_ThreeD_ProjectApp::Base_ThreeD_ProjectApp()
{}

/* 
 * @Description: deconstructor ( called when program exits )
 * @param: none
 * @return: none
 */
Base_ThreeD_ProjectApp::~Base_ThreeD_ProjectApp()
{
	delete mCam;
	delete mLight;
	delete mLightRef;
}

/* 
 * @Description: set basic app settings here
 * @param: Settings ( pointer to base app settings )
 * @return: none
 */
void Base_ThreeD_ProjectApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 720, 486 );		
	settings->setFrameRate( 60.0f );			//the more the merrier!
	settings->setResizable( false );			//this isn't going to be resizable
	
	//make sure secondary screen isn't blacked out as well when in fullscreen mode ( do wish it could accept keyboard focus though :(
	//settings->enableSecondaryDisplayBlanking( false );
}

/* 
 * @Description: setup function ( more aesthetic as could be in constructor here )
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::setup()
{
	RENDER_MODE = 3;
	
	glEnable( GL_LIGHTING );
	glEnable( GL_DEPTH_TEST );
	glEnable(GL_RESCALE_NORMAL); //important if things are being scaled as OpenGL also scales normals ( for proper lighting they need to be normalized )
	
	mParams = params::InterfaceGl( "3D_Scene_Base", Vec2i( 225, 125 ) );
	mParams.addParam( "Framerate", &mCurrFramerate, "", true );
	mParams.addParam( "Eye Distance", &mCameraDistance, "min=-100.0 max=-5.0 step=1.0 keyIncr== keyDecr=-");
	mParams.addParam( "Lighting On", &mLightingOn, "key=l");
	mParams.addParam( "Show/Hide Params", &mShowParams, "key=x");
	mParams.addSeparator();
    
	
	mCurrFramerate = 0.0f;
	mLightingOn = true;
	mViewFromLight = false;
	mShowParams = true;
	
	//create camera
	mCameraDistance = CAM_POSITION_INIT.z;
	mEye		= Vec3f(CAM_POSITION_INIT.x, CAM_POSITION_INIT.y, CAM_POSITION_INIT.z);
	mCenter		= Vec3f::zero();
	mUp			= Vec3f::yAxis();
	
	mCam = new CameraPersp( getWindowWidth(), getWindowHeight(), 180.0f );
	mCam->lookAt(mEye, mCenter, mUp);
	mCam->setPerspective( 45.0f, getWindowAspectRatio(), 1.0f, 50.0f );
	gl::setMatrices( *mCam );
	
	//create light
	mLight = new gl::Light( gl::Light::DIRECTIONAL, 0 );
	mLight->lookAt( Vec3f(LIGHT_POSITION_INIT.x, LIGHT_POSITION_INIT.y * -1, LIGHT_POSITION_INIT.z), Vec3f( 0, 0, 0 ) );
	mLight->setAmbient( Color( 1.0f, 1.0f, 1.0f ) );
	mLight->setDiffuse( Color( 1.0f, 1.0f, 1.0f ) );
	mLight->setSpecular( Color( 1.0f, 1.0f, 1.0f ) );
	mLight->setShadowParams( 100.0f, 1.0f, 20.0f );
	mLight->update( *mCam );
	mLight->enable();
	
	//create light ref
	mLightRef = new gl::Light( gl::Light::DIRECTIONAL, 0 );
	mLightRef->lookAt( LIGHT_POSITION_INIT, Vec3f( 0, 0, 0 ) );
	mLightRef->setShadowParams( 100.0f, 1.0f, 20.0f );
	
	//DEBUG Test objects
	ci::ColorA pink( CM_RGB, 0.84f, 0.49f, 0.50f, 1.0f );
	ci::ColorA green( CM_RGB, 0.39f, 0.78f, 0.64f, 1.0f );
	ci::ColorA blue( CM_RGB, 0.32f, 0.59f, 0.81f, 1.0f );
	ci::ColorA orange( CM_RGB, 0.77f, 0.35f, 0.35f, 1.0f );
	
	gl::Material torusMaterial;
	torusMaterial.setSpecular( ColorA( 1.0, 1.0, 1.0, 1.0 ) );
	torusMaterial.setDiffuse( pink );
	torusMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	torusMaterial.setShininess( 25.0f );
	
	gl::Material boardMaterial;
	boardMaterial.setSpecular( ColorA( 0.0, 0.0, 0.0, 0.0 ) );
	boardMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	boardMaterial.setDiffuse( green );	
	boardMaterial.setShininess( 0.0f );
	
	gl::Material boxMaterial;
	boxMaterial.setSpecular( ColorA( 0.0, 0.0, 0.0, 0.0 ) );
	boxMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	boxMaterial.setDiffuse( blue );	
	boxMaterial.setShininess( 0.0f );
	
	gl::Material sphereMaterial;
	sphereMaterial.setSpecular( ColorA( 1.0, 1.0, 1.0, 1.0 ) );
	sphereMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	sphereMaterial.setDiffuse( orange ) ;	
	sphereMaterial.setShininess( 35.0f );	
	
    //using DisplayLists for simplicity but highly recommend only use VBO's for serious work ( as DisplayLists will be deprecated soon ... and speed difference in now negligible )
	mTorus = gl::DisplayList( GL_COMPILE );
	mTorus.newList();
	gl::drawTorus( 1.0f, 0.3f, 32, 64 );
	mTorus.endList();
	mTorus.setMaterial( torusMaterial );
	
	mBoard = gl::DisplayList( GL_COMPILE );
	mBoard.newList();
	gl::drawCube( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 10.0f, 0.1f, 10.0f ) );
	mBoard.endList();
	mBoard.setMaterial( boardMaterial );
	
	mBox = gl::DisplayList( GL_COMPILE );
	mBox.newList();
	gl::drawCube( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 1.0f, 1.0f, 1.0f ) );
	mBox.endList();
	mBox.setMaterial( boxMaterial );
	
	mSphere = gl::DisplayList( GL_COMPILE );
	mSphere.newList();
	gl::drawSphere( Vec3f::zero(), 0.8f, 30 );
	mSphere.endList();
	mSphere.setMaterial( sphereMaterial );
	
    //noise texture required for SSAO calculations
	mRandomNoise = gl::Texture( loadImage( loadResource( NOISE_SAMPLER ) ) );
	
	initFBOs();
	initShaders();
}

/* 
 * @Description: update
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::update()
{
	mCurrFramerate = getAverageFps();
}

/* 
 * @Description: draw
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::draw()
{
    //clear depth and color every frame
	glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClearDepth(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	renderSceneToFBO();
	renderNormalsDepthToFBO();
	renderSSAOToFBO();
	
	renderScreenSpace();
	
	//glFinish(); //want to make sure everything is finished before jumping to UI ... slows down program big-time. Is totally unecessary ...
    
	if (mShowParams)
		params::InterfaceGl::draw();
}

/* 
 * @Description: render scene to FBO texture
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::renderSceneToFBO()
{
	mScreenSpace1.bindFramebuffer();

	glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClearDepth(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	if (mLightingOn)
		glDisable(GL_LIGHTING);
	glColor3f( 1.0f, 1.0f, 0.1f );
	gl::drawFrustum( mLightRef->getShadowCamera() );
	glColor3f( 1.0f, 1.0f, 1.0f );
	if (mLightingOn)
		glEnable(GL_LIGHTING);
	
	mEye = mCam->getEyePoint();
	mEye.normalize();
	mEye = mEye * abs(mCameraDistance);
	mCam->lookAt( mEye, mCenter, mUp );
	gl::setMatrices( *mCam );
	mLight->update( *mCam );
	
	drawTestObjects();
	
	mScreenSpace1.unbindFramebuffer();
	
	glDisable(GL_LIGHTING);
}

/* 
 * @Description: render scene normals to FBO ( required for SSAO calculations )
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::renderNormalsDepthToFBO()
{
	gl::setViewport( mNormalDepthMap.getBounds() );
	
	//render out main scene to FBO
	mNormalDepthMap.bindFramebuffer();
	
	glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClearDepth(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	mNormalDepthShader.bind();
		drawTestObjects();
	mNormalDepthShader.unbind();
	
	mNormalDepthMap.unbindFramebuffer();
	
	gl::setViewport( getWindowBounds() );
}

/* 
 * @Description: render SSAO now - woohoo!
 * @param: KeyEvent
 * @return: none
 */
void Base_ThreeD_ProjectApp::renderSSAOToFBO()
{
	gl::setViewport( mSSAOMap.getBounds() );
	
	//render out main scene to FBO
	mSSAOMap.bindFramebuffer();
	
	glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClearDepth(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	gl::setMatricesWindow( mSSAOMap.getSize() );
	
	mRandomNoise.bind(1);
	mNormalDepthMap.getTexture().bind(2);
	
	mSSAOShader.bind();
	
	mSSAOShader.uniform("rnm", 1 );
	mSSAOShader.uniform("normalMap", 2 );

    //look at shader and see you can set these through the client if you so desire.
//	mSSAOShader.uniform("rnm", 1 );
//	mSSAOShader.uniform("normalMap", 2 );	
//	mSSAOShader.uniform("totStrength", 1.38f);
//	mSSAOShader.uniform("strength", 0.07f);
//	mSSAOShader.uniform("offset", 10.0f);
//	mSSAOShader.uniform("falloff", 0.2f);
//	mSSAOShader.uniform("rad", 0.8f);

//	mSSAOShader.uniform("rnm", 1 );
//	mSSAOShader.uniform("normalMap", 2 );
//	mSSAOShader.uniform("farClipDist", 20.0f);
//	mSSAOShader.uniform("screenSizeWidth", (float)getWindowWidth());
//	mSSAOShader.uniform("screenSizeHeight", (float)getWindowHeight());
	
//	mSSAOShader.uniform("grandom", 1 );
//	mSSAOShader.uniform("gnormals", 2 );
	//mSSAOShader.uniform("gdepth", 1 );
	//mSSAOShader.uniform("gdiffuse", 1 );
		
		gl::drawSolidRect( Rectf( 0, 0, getWindowWidth(), getWindowHeight()) );
	
	mSSAOShader.unbind();
	
	mNormalDepthMap.getTexture().unbind(2);
	mRandomNoise.unbind(1);
	
	mSSAOMap.unbindFramebuffer();
	
	gl::setViewport( getWindowBounds() );
}

/* 
 * @Description: need to blur[the SSAO texture] horizonatally then vertically (for shader performance reasons). Called ping-ponging as it one FBO drawn to another
 * @param: KeyEvent
 * @return: none
 */
void Base_ThreeD_ProjectApp::pingPongBlur()
{
	//render horizontal blue first
	gl::setViewport( mPingPongBlurH.getBounds() );
	
	mPingPongBlurH.bindFramebuffer();
	
	glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClearDepth(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	gl::setMatricesWindow( mPingPongBlurH.getSize() );
	
	mSSAOMap.getTexture().bind(0);
	mHBlurShader.bind();
	mHBlurShader.uniform("RTScene", 0);
    gl::drawSolidRect( Rectf( 0, 0, getWindowWidth(), getWindowHeight()) );
	mHBlurShader.unbind();
	mSSAOMap.getTexture().unbind(0);
	
	mPingPongBlurH.unbindFramebuffer();
	
	//gl::setViewport( getWindowBounds() ); //redundant
	
	//now render vertical blur
	gl::setViewport( mPingPongBlurV.getBounds() );
	
	mPingPongBlurV.bindFramebuffer();
	
	glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClearDepth(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	gl::setMatricesWindow( mPingPongBlurV.getSize() );
	
	mPingPongBlurH.getTexture().bind(0);
	mHBlurShader.bind();
	mHBlurShader.uniform("RTBlurH", 0);
	gl::drawSolidRect( Rectf( 0, 0, getWindowWidth(), getWindowHeight()) );
	mHBlurShader.unbind();
	mPingPongBlurH.getTexture().unbind(0);
	
	mPingPongBlurV.unbindFramebuffer();
	
	gl::setViewport( getWindowBounds() );
}

/* 
 * @Description: render the final scene ( using all prior created FBOs and combine )
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::renderScreenSpace()
{	
	// use the scene we rendered into the FBO as a texture
	glEnable( GL_TEXTURE_2D );
	
	// show the FBO texture in the upper left corner
	gl::setMatricesWindow( getWindowSize() );
	
	switch (RENDER_MODE) 
	{
		case SHOW_STANDARD_VIEW:
		{
			mScreenSpace1.getTexture(0).bind(0);
            gl::drawSolidRect( Rectf( 0, getWindowHeight(), getWindowWidth(), 0) );
			mScreenSpace1.getTexture(0).unbind(0);
		}
            break;
			
		case SHOW_SSAO:
		{
			mSSAOMap.getTexture().bind(0);
			
			mBasicBlender.bind();
			
			mBasicBlender.uniform("ssaoTex", 0 );
			mBasicBlender.uniform("baseTex", 0 );
			gl::drawSolidRect( Rectf( 0, getWindowHeight(), getWindowWidth(), 0) );
			
			mBasicBlender.unbind();
			
			mSSAOMap.getTexture().unbind(0);
		}
            break;
			
		case SHOW_NORMALMAP:
		{
			mNormalDepthMap.getTexture().bind(0);
            gl::drawSolidRect( Rectf( 0, getWindowHeight(), getWindowWidth(), 0) );
			mNormalDepthMap.getTexture().unbind(0);
		}
            break;
			
		case SHOW_FINAL_SCENE:
		{
			pingPongBlur();
			
			gl::setMatricesWindow( getWindowSize() );
			
			mPingPongBlurV.getTexture().bind(0);
			mScreenSpace1.getTexture().bind(1);
			
			mBasicBlender.bind();
			
			mBasicBlender.uniform("ssaoTex", 0 );
			mBasicBlender.uniform("baseTex", 1 );
			gl::drawSolidRect( Rectf( 0, getWindowHeight(), getWindowWidth(), 0) );
			
			mBasicBlender.unbind();
			
			mScreenSpace1.getTexture().unbind(1);
			mPingPongBlurV.getTexture().unbind(0);
		}
			break;
	}
	
	glDisable(GL_TEXTURE_2D);
}

/* 
 * @Description: don't use but i like to have it available
 * @param: MouseEvent
 * @return: none
 */
void Base_ThreeD_ProjectApp::mouseDown( MouseEvent event )
{}

/* 
 * @Description: Listen for key presses
 * @param: KeyEvent
 * @return: none
 */
void Base_ThreeD_ProjectApp::keyDown( KeyEvent event ) 
{
	//printf("%i \n", event.getCode());
	
	switch ( event.getCode() ) 
	{
		case KeyEvent::KEY_1:
		{
			RENDER_MODE = 0;
		}
			break;
		case KeyEvent::KEY_2:
		{
			RENDER_MODE = 1;
		}
			break;
		case KeyEvent::KEY_3:
		{
			RENDER_MODE = 2;
		}
			break;
		case KeyEvent::KEY_4:
		{
			RENDER_MODE = 3;
		}
			break;		
			
		case KeyEvent::KEY_UP:
		{
			Vec3f lightPos = mLight->getPosition();
			if ( lightPos.y > 0 )
				lightPos.y *= -1;
			lightPos = lightPos + Vec3f(0, 0.0, 0.1);		
			mLight->lookAt( lightPos, Vec3f::zero() );
			//mLight->update( *mCam );
			
			lightPos = mLightRef->getPosition() + Vec3f(0, 0.0, 0.1);
			mLightRef->lookAt( lightPos, Vec3f::zero() );
			mLightRef->update( *mCam );
		}
			break;
		case KeyEvent::KEY_DOWN:
		{
			Vec3f lightPos = mLight->getPosition();
			if ( lightPos.y > 0 )
				lightPos.y *= -1;
			lightPos = lightPos + Vec3f(0, 0.0, -0.1);		
			mLight->lookAt( lightPos, Vec3f::zero() );
			//mLight->update( *mCam );
			
			lightPos = mLightRef->getPosition() + Vec3f(0, 0.0, -0.1);	
			mLightRef->lookAt( lightPos, Vec3f::zero() );
			mLightRef->update( *mCam );
		}
			break;
		case KeyEvent::KEY_LEFT:
		{
			Vec3f lightPos = mLight->getPosition();
			if ( lightPos.y > 0 )
				lightPos.y *= -1;
			lightPos = lightPos + Vec3f(0.1, 0, 0);		
			mLight->lookAt( lightPos, Vec3f::zero() );
			//mLight->update( *mCam );
			
			lightPos = mLightRef->getPosition() + Vec3f(0.1, 0, 0);
			mLightRef->lookAt( lightPos, Vec3f::zero() );
			mLightRef->update( *mCam );
		}
			break;
		case KeyEvent::KEY_RIGHT:
		{
			Vec3f lightPos = mLight->getPosition();
			if ( lightPos.y > 0 )
				lightPos.y *= -1;
			lightPos = lightPos + Vec3f(-0.1, 0, 0);		
			mLight->lookAt( lightPos, Vec3f::zero() );
			//mLight->update( *mCam );
			
			lightPos = mLightRef->getPosition() + Vec3f(-0.1, 0, 0);	
			mLightRef->lookAt( lightPos, Vec3f::zero() );
			mLightRef->update( *mCam );
		}
			break;
		case KeyEvent::KEY_w:
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(1, 0, 0), -0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			//mLight->update( *mCam );
			mLightRef->update( *mCam );
		}
			break;
		case KeyEvent::KEY_a:
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(0, 1, 0), 0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			//mLight->update( *mCam );
			mLightRef->update( *mCam );
		}
			break;	
		case KeyEvent::KEY_s:
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(1, 0, 0), 0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			//mLight->update( *mCam );
			mLightRef->update( *mCam );
		}
			break;
		case KeyEvent::KEY_d:
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(0, 1, 0), -0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			//mLight->update( *mCam );
			mLightRef->update( *mCam );
		}
			break;
		default:
			break;
	}
}

/* 
 * @Description: drawing all objects in scene here
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::drawTestObjects()
{
	//glColor3f(1.0, 0.2, 0.2);
	gl::pushMatrices();
	glTranslatef(-2.0f, -1.0f, 0.0f);
	glRotated(90.0f, 1, 0, 0);
		mTorus.draw();
	gl::popMatrices();
	
	//glColor3f(0.4, 1.0, 0.2);
	gl::pushMatrices();
	glTranslatef(0.0f, -1.35f, 0.0f);
		mBoard.draw();
	gl::popMatrices();
	
	//glColor3f(0.8, 0.5, 0.2);
	gl::pushMatrices();
	glTranslatef(0.4f, -0.3f, 0.5f);
	glScalef(2.0f, 2.0f, 2.0f);
		mBox.draw();
	gl::popMatrices();
	
	//glColor3f(0.3, 0.5, 0.9);
	gl::pushMatrices();
	glTranslatef(0.1f, -0.56f, -1.25f);
		mSphere.draw();
	gl::popMatrices();
}

/* 
 * @Description: initialize all shaders here
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::initShaders()
{
	mSSAOShader			= gl::GlslProg( loadResource( SSAO_VERT ), loadResource( SSAO_FRAG_LIGHT ) );
	mNormalDepthShader	= gl::GlslProg( loadResource( NaDepth_VERT ), loadResource( NaDepth_FRAG ) );
	mBasicBlender		= gl::GlslProg( loadResource( BBlender_VERT ), loadResource( BBlender_FRAG ) );
	mHBlurShader		= gl::GlslProg( loadResource( BLUR_H_VERT ), loadResource( BLUR_H_FRAG ) );
	mVBlurShader		= gl::GlslProg( loadResource( BLUR_V_VERT ), loadResource( BLUR_V_FRAG ) );
}

/* 
 * @Description: initialize all FBOs here
 * @param: none
 * @return: none
 */
void Base_ThreeD_ProjectApp::initFBOs()
{		
    //being lazy so settings are set to high for all texture ( generally the only one that needs to be 16-bit is the SSAO FBO so that no moire precision problems are seen )
	gl::Fbo::Format format;
	//format.setDepthInternalFormat( GL_DEPTH_COMPONENT32 );
	format.setColorInternalFormat( GL_RGBA16F_ARB );
	format.setSamples( 4 ); // enable 4x antialiasing
    
	//init screen space render
	mScreenSpace1	= gl::Fbo( getWindowWidth(), getWindowHeight(), format );
	mNormalDepthMap	= gl::Fbo( getWindowWidth()/2.0f, getWindowHeight()/2.0f, format );
	mPingPongBlurH	= gl::Fbo( getWindowWidth()/2.0f, getWindowHeight()/2.0f, format );
	mPingPongBlurV	= gl::Fbo( getWindowWidth()/2.0f, getWindowHeight()/2.0f, format );
	mFinalScreenTex = gl::Fbo( getWindowWidth(), getWindowHeight(), format );
	mSSAOMap		= gl::Fbo( getWindowWidth()/2.0f, getWindowHeight()/2.0f, format );
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );	
}

CINDER_APP_BASIC( Base_ThreeD_ProjectApp, RendererGl )
