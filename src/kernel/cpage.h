// vim:tabstop=4:shiftwidth=4:noexpandtab:textwidth=80
/*
 * =====================================================================================
 *        Filename:  cpage.cc
 *     Description:  CPage.
 *         Created:  20/03/2006 11:46:14 AM CET
 *          Author:  jmisutka, mhocko (annotation stuff)
 * =====================================================================================
 */

#ifndef _CPAGE_H
#define _CPAGE_H

// all basic includes
#include "static.h"

// CDict
#include "cobject.h"
// CContentstream
#include "ccontentstream.h"
// CAnnotation
#include "cannotation.h"


//=====================================================================================
namespace pdfobjects {
//=====================================================================================


//=====================================================================================
// Display parameters (loose xpdf parameters put into a simple structure)
// 	--  default values are in cpage.cc because we do not want to have global variables.
//=====================================================================================

/** 
 * Graphical state parameters. 
 *
 * These parameters are used by xpdf when updating bounding boxex of content stream operators,
 * displaying page etc.
 */
typedef struct DisplayParams
{
	/** Paramaters */
	double 		hDpi;		/**< Horizontal DPI. */
	double 		vDpi; 		/**< Vertical DPI. 	*/
	Rectangle 	pageRect;	/**< Page rectangle. */
	int 		rotate;		/**< Page rotation. 	*/
	GBool		useMediaBox;/**< Use page media box. */
	GBool		crop;		/**< Crop the page. 	*/
	GBool		upsideDown;	/**< Upside down. 	*/
	
	/** Constructor. Default values are set. */
	DisplayParams () : 
		hDpi (DEFAULT_HDPI), vDpi (DEFAULT_VDPI),
		pageRect (Rectangle (DEFAULT_PAGE_LX, DEFAULT_PAGE_LY, DEFAULT_PAGE_RX, DEFAULT_PAGE_RY)),
		rotate (DEFAULT_ROTATE), useMediaBox (gTrue), crop (gFalse), upsideDown (gTrue) 
		{}



	/** Equality operator. */
	bool operator== (const DisplayParams& dp) const
	{
		return (hDpi == dp.hDpi && vDpi == dp.vDpi &&
				pageRect == dp.pageRect && rotate == dp.rotate &&
				useMediaBox == dp.useMediaBox && crop == dp.crop &&
				upsideDown == dp.upsideDown);
	}

	/** Converting position from pixmap of viewed page to pdf position.
	 * @param fromX	X position on viewed page.
	 * @param fromY	Y position on viewed page.
	 *
	 * @param toX	return X position in pdf page.
	 * @param toY	return Y position in pdf page.
	 *
	 * @see convertPdfPosToPixmapPos
	 */
	void convertPixmapPosToPdfPos( double fromX, double fromY, double & toX, double & toY ) const {
		double * ctm /*[6]*/;
		double h;
		PDFRectangle pdfRect ( pageRect.xleft, pageRect.yleft, pageRect.xright, pageRect.yright );
		GfxState state (hDpi, vDpi, &pdfRect, rotate, upsideDown );
		ctm = state.getCTM();

		h = (ctm[0]*ctm[3] - ctm[1]*ctm[2]);

		assert( h != 0 );

		toX = (fromX*ctm[3] - ctm[2]*fromY + ctm[2]*ctm[5] - ctm[4]*ctm[3]) / h;
		toY = (ctm[0]*fromY + ctm[1]*ctm[4] - ctm[0]*ctm[5] - ctm[1]*fromX) / h;
	}

	/** Converting pdf position to position on pixmap of viewed page.
	 * @param fromX	X position in pdf page.
	 * @param fromY	Y position in pdf page.
	 *
	 * @param toX	return X position on viewed page.
	 * @param toY	return Y position on viewed page.
	 *
	 * @see convertPixmapPosToPdfPos
	 */
	void convertPdfPosToPixmapPos( double fromX, double fromY, double & toX, double & toY ) const {
		PDFRectangle pdfRect ( pageRect.xleft, pageRect.yleft, pageRect.xright, pageRect.yright );
		GfxState state (hDpi, vDpi, &pdfRect, rotate, upsideDown );

		state.transform( fromX, fromY, &toX, &toY );
	}

	//
	// Default values
	// -- small hack to declare them as ints, to be able to init
	// them here (if double, we could not init them here because of the non
	// integral type compilator error))
	// 
	static const int DEFAULT_HDPI 	= 72;		/**< Default horizontal dpi. */
	static const int DEFAULT_VDPI 	= 72;		/**< Default vertical dpi. */
	static const int DEFAULT_ROTATE	= 0;		/**< No rotatation. */

	static const int DEFAULT_PAGE_LX = 0;		/**< Default x position of left upper corner. */
	static const int DEFAULT_PAGE_LY = 0;		/**< Default y position of right upper corner. */
	static const int DEFAULT_PAGE_RX = 612;		/**< Default A4 width on a device with 72 horizontal dpi. */
	static const int DEFAULT_PAGE_RY = 792;		/**< Default A4 height on a device with 72 vertical dpi. */

} DisplayParams;


//=====================================================================================
// Text search parameters (loose xpdf parameters put into a simple structure)
// 	--  default values are in cpage.cc because we do not want to pollute global space.
//=====================================================================================

/** 
 * Text search parameters. 
 *
 * These parameters are used by xpdf when serching a text string.
 */
typedef struct TextSearchParams
{
	/** Paramaters */
	GBool startAtTop;		/**< Start searching from the top.    */
	double xStart; 			/**< Start searching from x position. */
	double yStart; 			/**< Start searching from y position. */
	double xEnd; 			/**< Stop searching from x position.  */
	double yEnd; 			/**< Stop searching from y position.  */

	/** Constructor. Default values are set. */
	TextSearchParams () : 
		startAtTop (DEFAULT_START_AT_TOP),
		xStart (DEFAULT_X_START), yStart (DEFAULT_Y_START), xEnd (DEFAULT_X_END), yEnd (DEFAULT_Y_END)
	{}

	//
	// Default values  
	// -- small hack to declare them as ints, to be able to init
	// them here (if double, we could not init them here because of the non
	// integral type compilator error))
	//
	static const GBool DEFAULT_START_AT_TOP 	= gTrue;	/**< Start at top. */

	static const int DEFAULT_X_START = 0;	/**< Default x position of left upper corner. */
	static const int DEFAULT_Y_START = 0;	/**< Default y position of left upper corner. */
	static const int DEFAULT_X_END = 0;		/**< Default x position of right upper corner. */
	static const int DEFAULT_Y_END = 0;		/**< Default y position of right upper corner. */


} TextSearchParams;


//=====================================================================================
// Comparators Point/Rectangle
//=====================================================================================

/** 
 * Comparator that we can use to find out if another rectangle intersects
 * rectangle specified by this comparator.
 */
struct PdfOpCmpRc
{
	/** 
	 * Consructor. 
	 *
	 * @param rc Rectangle used when comparing.
	 */
	PdfOpCmpRc (const Rectangle& rc) : rc_(rc) {};
	
	/** 
	 * Is the intersection of supplied rectangle and rectangle specified in the
	 * constructor not empty. 
	 *
	 * Our rectangle does NOT contain another rectangle if
	 * <ul>
	 * 	<li>min (xleft-our, xright-our) >= min (xleft, xright)</li>
	 * 	<li>max (xleft-our, xright-our) <= max (xleft, xright)</li>
	 * 	<li>min (yleft-our, yright-our) >= min (yleft, yright)</li>
	 * 	<li>max (yleft-our, yright-our) <= max (yleft, yright)</li>
	 * </ul>
	 */
	bool operator() (const Rectangle& rc) const
	{
		if ( std::min(rc_.xleft,rc_.xright) >= std::min(rc.xleft, rc.xright) ) {
			return false;
		}
		if ( std::max(rc_.xleft,rc_.xright) <= std::max(rc.xleft, rc.xright) ) {
			return false;
		}

		if ( std::min(rc_.yleft,rc_.yright) >= std::min(rc.yleft, rc.yright) ) {
			return false;
		}
		if ( std::max(rc_.yleft,rc_.yright) <= std::max(rc.yleft, rc.yright) ) {
			return false;
		}

		return true;
	}

private:
	const Rectangle rc_;	/**< Rectangle to be compared. */
};


/** 
 * Comparator that we can use to find out if a rectange contains point specified
 * by this comparator.
 */
struct PdfOpCmpPt
{
	/** 
	 * Consructor. 
	 * 
	 * @param pt Point that we use when comparing.
	 */
	PdfOpCmpPt (const Point& pt) : pt_(pt) {};
	
	/** 
	 * Is point in a rectangle. 
	 * 
	 * @param rc Rectangle.
	 */
	bool operator() (const Rectangle& rc) const
	{
		return (rc.contains (pt_.x, pt_.y));
	}

private:
	const Point pt_;	/**< Point to be compared. */
};

/** Sets unitialized inheritable page attributes.
 * @param pageDict Page dictionary reference where to set values.
 *
 * Gets InheritedPageAttr structure for given pageDict (uses
 * fillInheritedPageAttr helper function) and sets all fields which are not
 * present in given dictionary to found values.
 */
void setInheritablePageAttr(boost::shared_ptr<CDict> & pageDict);

//=====================================================================================
// CPage
//=====================================================================================

//
// Forward declaration
//
class CPage;


//
//
//
typedef observer::ObserverHandler<CPage> CPageObserverSubject;

/**
 * This object represents page object from pdf specification v1.5. Pdf page object is a dictionary
 * reachable from page tree structure with several required properties. 
 * It is responsible just for one single page.
 *
 * Every pdf page contains all information required for displaying the page
 * (e.g. page metrics, page contents etc.) Page properties can be inherited from
 * its parent in the page tree. The first encountered during page tree traversal
 * is used. This feature can
 * cause problems because it is no well defined what does it mean to change a
 * property that is inherited (it is not present in the page dictionary but in a
 * parent)
 *
 * We display a page using xpdf code. The argument to this function is an output
 * device which can draw graphical objects. The contents of a page is specified
 * by a "Contents" entry in the page dictionary. If empty the page is blank.
 * 
 * CPage is a subject that can be observed. This is important when a change
 * leads to content stream reparsing (e.g. deleting an entry from "Contents"
 * property in the page dictionary)
 *
 * Content stream consists of a sequence of operators which should be processed
 * sequentially. The operators define what is really on a page. The pdf
 * specification is too general about pdf operators and that is why working with
 * operators is difficult. According to pdf specification text is split
 * neither to sentences nor words. Letters of a word can occur randomly in the content stream
 * because the position of a letter (text) is absolute. (e.g. it is very likely
 * that a word "humor" will be split into "hu" "m" "or" because of the "m"
 * beeing wider than other letters.) This makes searching and exporting page text a problem. 
 * We use xpdf code to perform both actions. Xpdf parses a page to lines and
 * words with a rough approuch when a more letters are claimed as one word when
 * they are close enough. This algorithm is working ok for normal pdf files, but
 * if the pdf creator would like to disable text exporting it could produce such
 * sequence of pdfoperators, that hardly any program could export text correctly.
 *
 * Pdf operators are in one or more streams. Problem with this
 * approach is that these operators can be split
 * into streams at almost arbitrary place.
 *
 * Processing pdf operators can be very expensive so they are parsed only on demand. Each operator 
 * can be placed in a bounding box. These bounding boxes are used when searching
 * the page for a text, selecting objects, drawing the page. 
 *
 * Each page content stream is a selfcontained entity that can not
 * use resources defined in another page. It can use only inherited resources
 * from a parent in the page tree. Which means we can not simply change fonts
 * on a page to match another page, use images from another page etc.
 */
class CPage : public noncopyable, public CPageObserverSubject
{
public:
	/** Container of content streams. */
	typedef std::vector<boost::shared_ptr<CContentStream> > ContentStreams;
	/** Type for annotation storage. */
	typedef std::vector<boost::shared_ptr<CAnnotation> > AnnotStorage;
	/** Position in content stream container. */
	typedef size_t CcPosition;
	
	/** Type of page observer context. */
	typedef observer::BasicChangeContext<CPage> BasicObserverContext;

private:

	/** Pdf dictionary representing a page. */
	boost::shared_ptr<CDict> dictionary;

	/** Class representing content stream. */
	ContentStreams contentstreams;

	/** Actual display parameters. */
	DisplayParams lastParams;

	/** Is page valid. */
	bool valid;
	
	/** Keeps all annotations from this page.
	 *
	 * This structure is synchronized with page dictionary Annots field with
	 * observer.
	 */
	AnnotStorage annotStorage;

	//
	// Constructors
	//
public:
		
	/** 
	 * Constructor. 
	 * 
	 * @param pageDict Dictionary representing pdf page.
	 */
	CPage (boost::shared_ptr<CDict>& pageDict);

	
	//
	// Annotation observer
	//
private:

	/** 
	 * Consolidates annotStorage field according given change.
	 * Works in two steps. First handles oldValue and second newValue. At first
	 * checkes oldValue type and if it is reference, dereference indirect
	 * objects and if it is annotation dictionary, it will invalidate 
	 * associated CAnnotation and removes it from annotStorage. 
	 * <br>
	 * In second step, checks newValue type and if it is reference to
	 * dictionary, it will create new CAnnotation instance and adds it to
	 * annotStorage. 
	 * 
	 * @param oldValue Removed reference from annotStorage.
	 * @param newValue Added reference to the annotStorage.
	 */
	void consolidateAnnotsStorage(boost::shared_ptr<IProperty> & oldValue, boost::shared_ptr<IProperty> & newValue);
	
	/** Observer for Annots property.
	 * This observer is registered on page dictionary and if Annots property is
	 * a reference also to this property. Any change which leads to change of 
	 * Annots array (either add, remove or change) is handled here.  Note that
	 * it doesn't handle array content change.
	 */
	class AnnotsPropWatchDog: public IIPropertyObserver
	{
		/** Page owner of this observer.
		 */
		CPage* page;

	public:
		/** Initialization constructor.
		 * Sets page field according parameter.
		 *
		 * @param _page CPage instance.
		 */
		AnnotsPropWatchDog(CPage * _page):page(_page)
		{
			// given parameter must be non NULL
			// this is used only internaly by CPage, so assert is enough for
			// checking
			assert(_page);
		}

		/** Empty destructor.
		 */
		virtual ~AnnotsPropWatchDog() throw(){}; 
		
		/** Observer handler.
		 * 
		 * Checks given context type:
		 * <ul>
		 * <li>BasicChangeContext means that Annots property in page dictionary
		 * is reference and its value has changed.
		 * <li>ComplexChangeContext means that page dictionary has changed. So
		 * checks property id and if it not Annots, immediatelly returns,
		 * because this change doesn't affect annotations. Otherwise checks
		 * original value type. If it is reference, unregisters this observer
		 * from it. If newValue is reference, registers observer to it.
		 * </ul>
		 * In any case:
		 * <ul>
		 * <li>Tries to get array from oldValue and unregister observers from
		 * it (uses page-&gt;unregisterAnnotsObservers).
		 * <li>Invalidates and removes all annotations from 
		 * page-&gt;annotStorage.
		 * <li>collects all current annotations (uses collectAnnotations).
		 * <li>Tries to get current Annots array and registers observers to it
		 * (uses page-&gt;registerAnnotsObservers)
		 * </ul>
		 *
		 * @param newValue New value of changed property.
		 * @param context Context of the change.
		 */
		virtual void notify (boost::shared_ptr<IProperty> newValue, 
							 boost::shared_ptr<const IProperty::ObserverContext> context) const throw();

		/** Returns observer priority.
		 */
		virtual priority_t getPriority()const throw()
		{
			// TODO some constant
			return 0;
		}
	};
	
	/** 
	 * Observer for Annots array synchronization.
	 * This observer is registered on Annots array property and all its
	 * reference typed elements. It handles change in Annots array content -
	 * this means either element is added, removed or replaced, or any of its
	 * reference elements changes its value.
	 */
	class AnnotsArrayWatchDog: public IIPropertyObserver
	{
		/** Page owner of this observer.
		 */
		CPage* page;

	public:
		typedef observer::BasicChangeContext<IProperty> BasicObserverContext;
		typedef CDict::CDictComplexObserverContext ComplextObserverContext;
			
		/** Initialization constructor.
		 * Sets page field according parameter.
		 *
		 * @param _page CPage instance.
		 */
		AnnotsArrayWatchDog(CPage * _page):page(_page)
		{
			// given parameter must be non NULL
			// this is used only internaly by CPage, so assert is enough for
			// checking
			assert(_page);
		}

		/** Empty destructor.
		 */
		virtual ~AnnotsArrayWatchDog() throw(){}; 
		
		/** Observer handler.
		 * 
		 * Checks given context type:
		 * <ul>
		 * <li>BasicObserverContext means that Annots array reference element 
		 * has changed its value.
		 * <li>ComplexChangeContext means that Annots array content has changed.
		 * If original value is reference, then unregisters this obsever from
		 * it. If newValue is reference registers this observer to it.
		 * <li>Different context is not supported and so method immediatelly
		 * returns.
		 * </ul>
		 * In both situations calls consolidateAnnotsStorage with original and
		 * new value parameters.
		 *
		 * @param newValue New value of changed property.
		 * @param context Context of the change.
		 */
		virtual void notify (boost::shared_ptr<IProperty> newValue, 
							 boost::shared_ptr<const IProperty::ObserverContext> context) const throw();

		/** Returns observer priority.
		 */
		virtual priority_t getPriority()const throw()
		{
			// TODO some constant
			return 0;
		}
	};

	/** Watchdog for Annots property.
	 * @see AnnotsPropWatchDog
	 */
	boost::shared_ptr<AnnotsPropWatchDog> annotsPropWatchDog;

	/** Watchdog for Annotation array.
	 * @see AnnotsArrayWatchDog
	 */
	boost::shared_ptr<AnnotsArrayWatchDog> annotsArrayWatchDog;

	/** Registers observers for annotations synchronization.
	 * Checks type of given property and if it is reference, registers
	 * annotsPropWatchDog observer to it and dereferences indirect object. If 
	 * annots or dereferenced object is array, registers annotsArrayWatchDog 
	 * observer to it and all its reference type elements.
	 *
	 * @param annots Annots property.
	 */
	void registerAnnotsObservers(boost::shared_ptr<IProperty> & annots);

	/** Unregisters obsevers from given Annots property.
	 * This method works reversely to registerAnnotsObservers (observers are
	 * unregistered but rest is same).
	 * 
	 * @param annots Annots property.
	 */
	void unregisterAnnotsObservers(boost::shared_ptr<IProperty> & annots);


	//
	// CStream observer
	//
private:
	/** 
	 * Observer implementation for content stream synchronization.
	 */
	class ContentsWatchDog: public IIPropertyObserver
	{
		/** 
		 * Owner of this observer.
		 */
		CPage* page;

	public:
		/** Initialization constructor.
		 * Sets page field according parameter.
		 * 
		 * @param _page CPage instance.
		 */
		ContentsWatchDog (CPage* _page) : page(_page)
			{ assert(_page); }

		/** Empty destructor.  */
		virtual ~ContentsWatchDog() throw() {}; 
		
		/** 
		 * Observer handler.
		 * 
		 * @param 
		 * @param context Context of the change.
		 */
		virtual void notify (boost::shared_ptr<IProperty>, 
							 boost::shared_ptr<const IProperty::ObserverContext>) const throw();

		/** Returns observer priority. */
		virtual priority_t getPriority() const throw()
			{ return 0;	}
	};
	
	/**
	 * Contents observer.
	 */
	boost::shared_ptr<ContentsWatchDog> contentsWatchDog;

protected:
	/**
	 * Register observer on all cstreams that content stream consists of.
	 */
	void registerContentsObserver () const;

	/**
	 * Unregister observer from all cstreams that this object consists of.
	 *
	 * This function is called when saving consten stream consisting of
	 * more streams. If we do not unregister observers, we would be notified
	 * that a stream has changed after the first save (when the content stream
	 * is invalid) and our observer would want to reparse an invalid stream.
	 */
	void unregisterContentsObserver () const;

	//
	// Destructor
	//
public:
	
	/** Destructor. */
	~CPage () 
	{ 
		kernelPrintDbg (debug::DBG_INFO, "Page destroyed."); 
	
		// Unregister all observers
		if (valid)
		{
			// Unregister contents observer
			unregisterContentsObserver ();
			// unregisters annotation observers - if annotation array present in
			// page dictionary
			if(dictionary->containsProperty("Annots"))
			{
				boost::shared_ptr<IProperty> annotsDict=dictionary->getProperty("Annots");
				unregisterAnnotsObservers(annotsDict);
			}
		}
	};

	
	//
	// Comparable interface
	//
public:
	/** 
	 * Equality operator. 
	 *
	 * @param page Another page object.
	 */
	bool operator== (const CPage& page)
	{
		return (this == &page) ? true : false;
	};

	
	//
	// Invalidate page
	//
public:
	/**
	 * Inform all obsevers that this page is not valid.
	 */
	void invalidate ()
	{ 
		assert (valid);
		// unregisters annotation observers - if annotation array present in
		// page dictionary
		if(dictionary->containsProperty("Annots"))
		{
			boost::shared_ptr<IProperty> annotsDict=dictionary->getProperty("Annots");
			unregisterAnnotsObservers(annotsDict);
		}
		// Unregister contents observer
		unregisterContentsObserver ();
		_objectChanged (true); 
		valid = false;
	};
	
	//
	// Get methods
	//	
public:
	
	/**
	 * Get the dictionary representing this object.
	 *
	 * @return Dictionary.
	 */
	boost::shared_ptr<CDict> getDictionary () const { return dictionary; };
	
	
	/**
	 * Get pdf operators at specified position.
	 * This call will be delegated to content stream object.
	 *
	 * @param opContainer Operator container where operators in specified are
	 * 						wil be stored.
	 * @param rc 		Rectangle around which we will be looking.
	 */
	template<typename OpContainer>
	void getObjectsAtPosition  (OpContainer& opContainer, const Rectangle& rc)
	{	
		kernelPrintDbg (debug::DBG_DBG, " at rectangle (" << rc << ")");
		// Get the objects with specific comparator
		getObjectsAtPosition (opContainer, PdfOpCmpRc (rc));
	}
	
	
	/**
	 * Get pdf operators at specified position. 
	 * This call will be delegated to content stream object.
	 * 
	 * @param opContainer Operator container where operators in specified are
	 * 						wil be stored.
	 * @param pt 		Point around which we will be looking.
	 */
	template<typename OpContainer>
	void getObjectsAtPosition  (OpContainer& opContainer, const Point& pt)
	{	
		kernelPrintDbg (debug::DBG_DBG, " at point (" << pt << ")");
		// Get the objects with specific comparator
		getObjectsAtPosition (opContainer, PdfOpCmpPt (pt));
	}

	
	/**
	 * Get pdf operators at specified position. 
	 * This call will be delegated to content stream object.
	 *
	 * @param opContainer Operator container where operators in specified are wil be stored.
	 * @param cmp 	Null if default kernel area comparator should be used otherwise points 
	 * 				 to an object which will decide whether an operator is "near" a point.
	 */
	template<typename OpContainer, typename PositionComparator>
	void getObjectsAtPosition (OpContainer& opContainer, PositionComparator cmp)
	{	
		kernelPrintDbg (debug::DBG_DBG, "");
		
		// Are we in valid pdf
		assert (hasValidPdf (dictionary));
		assert (hasValidRef (dictionary));
		if (!hasValidPdf(dictionary) || !hasValidRef(dictionary))
			throw CObjInvalidObject ();

		// If not parsed
		if (contentstreams.empty())
			parseContentStream ();		
	
		// Get the objects with specific comparator
		for (ContentStreams::iterator it = contentstreams.begin (); it != contentstreams.end(); ++it)
			(*it)->getOperatorsAtPosition (opContainer, cmp);
	}

	/** 
	 * Get contents streams.
	 *
	 * @param container Output container of all contentstreams.
	 *
	 * @return Content stream.
	 */
	template<typename Container>
	void
	getContentStreams (Container& container)
	{
		kernelPrintDbg (debug::DBG_DBG, "");
		assert (valid);

		// If not parsed
		if (contentstreams.empty())
			parseContentStream ();		

		container.clear();
		std::copy (contentstreams.begin(), contentstreams.end(), std::back_inserter(container));
	}

	
	/**  
	 * Returns plain text extracted from a page using xpdf code.
	 * 
	 * This method uses xpdf TextOutputDevice that outputs a page to a text device.
	 * Text in a pdf is stored neither word by word nor letter by letter. It is not
	 * easy not decide whether two letters form a word. Xpdf uses insane
	 * algorithm that works most of the time.
	 *
	 * @param text Output string  where the text will be saved.
	 * @param encoding Encoding format.
	 * @param rc Rectangle from which to extract the text.
	 */
 	void getText (std::string& text, const std::string* encoding = NULL, const Rectangle* rc = NULL)  const;

	//
	// Annotations
	//
public:
	/** 
	 * Fills given container with all page's annotations.
	 * 
	 * Copies annotStorage content to given container (which is cleared at
	 * first).
	 * <br>
	 * Given container must support clear and insert operations and store
	 * shared_ptr&lt;CAnnotation$gt; elements. 
	 * 
	 * @param container Container which is filled in.
	 */
	template<typename T>
	void getAllAnnotations(T  & container)const
	{
		assert (valid);
		container.clear();	
		container.insert(container.begin(), annotStorage.begin(), annotStorage.end());
	}

	/** 
	 * Adds new annotation to this page.
	 * 
	 * Inserts deep copy of given annotation and stores its reference to Annots
	 * array in page dictionary (if this doesn't exist, it is created). 
	 * User has to call getAllAnnotations to get current annotations state (we 
	 * don't have identifier for annotations - there are some mechanisms how to 
	 * do it according pdf specification, but there is no explicit identifier).
	 * <br>
	 * Given annotation may come from different CPdf or may belong to nowhere.
	 * <br>
	 * As a result annotStorage is updated. New indirect object representing
	 * annotation dictionary is added to same pdf (dictionary is same as given
	 * one except P field is updated to contain correct reference to this page).
	 * <br>
	 * Note that this page must belong to pdf and has to have valid indirect
	 * reference. This is neccessary, because annotation is indirect object page
	 * keeps reference to it. Reference without pdf doesn't make sense.
	 *
	 * @param annot Annotation to add.
	 * @throw CObjInvalidObject if this page doesn't have valid pdf or indirect
	 * reference.
	 * @throw ElementBadTypeException if Annots field from page dictionary is
	 * not an array (or reference with array indirect target).
	 */ 
	void addAnnotation(boost::shared_ptr<CAnnotation> annot);

	/** Removes given annotation from page.
	 * @param annot Annotation to remove.
	 *
	 * Tries to find given annotation in annotStorage and if found, removes
	 * reference from Annots array.
	 * <br>
	 * As a result, removed annotation is invalidated and not accessible. User 
	 * has to call getAllAnnotations method to get current state (same way as 
	 * in addAnnotation case).
	 *
	 * @return true if annotation was removed.
	 */
	bool delAnnotation(boost::shared_ptr<CAnnotation> annot);


	//
	// Font 
	//
public:
	/**
	 * Get all font ids and base names that are in the resource dictionary of a page.
	 *
	 * The resource can be inherited from a parent in the page tree dictionary.
	 * Base names should be human readable or at least standard system fonts
	 * defined in the pdf specification. We
	 * must choose from these items to make a font change valid. Otherwise, we
	 * have to add standard system font or manually a font object.
	 *
	 * @param cont Output container of font id and basename pairs.
	 */
	template<typename Container>
	void getFontIdsAndNames (Container& cont) const;
	
	/**
	 * Add new simple type 1 font item to the page resource dictionary. 
	 *
	 * The id of this font is arbitrary but it has to be unique. It will be generated as follows: 
	 * PdfEditor for the first item, PdfEditorr for the second, PdfEditorrr for
	 * the third etc.
	 *
	 * We supposed that the font name is a standard system font avaliable to all viewers.
	 *
	 * @param fontname Output container of pairs of (Id,Name).
	 * @param winansienc Set encoding to standard WinAnsiEnconding.
	 */
	void addSystemType1Font (const std::string& fontname, bool winansienc = true);


	//
	// Helper methods
	//
public:	
	/**
	 * Draw page on an output device.
	 *
	 * We use xpdf code to draw a page. It uses insane global parameters and
	 * many local parameters.
	 *
	 * @param out Output device.
 	 * @param params Display parameters.
	 */
	void displayPage (::OutputDev& out, const DisplayParams params, int x = -1, int y = -1, int w = -1, int h = -1); 
	
	/**
	 * Draw page on an output device with last used display parameters.
	 *
	 * @param out Output device.
	 * @param dict If not null, page is created from dict otherwise
	 * this page dictionary is used. But still some information is gathered from this page dictionary.
	 */
	void displayPage (::OutputDev& out, boost::shared_ptr<CDict> dict = boost::shared_ptr<CDict> (), int x = -1, int y = -1, int w = -1, int h = -1) const;

	/**
	 * Parse content stream. 
	 * Content stream is an optional property. When found it is parsed,
	 * nothing is done otherwise.
	 *
	 * @return True if content stream was found and was parsed, false otherwise.
	 */
	bool parseContentStream ();

	/**
	 * Reparse content stream using actual display parameters. 
	 */
	void reparseContentStream ();


	/**
	 * Add new content stream to the front. This function adds new entry in the "Contents"
	 * property of a page. The container of provided operators must form a valid
	 * contentstream.
	 * This function should be used when supplied operators
	 * should be handled at the beginning end e.g. should be drawn first which means
	 * they will appear the "below" other object.
	 *
	 * This function can be used to separate our changes from original content stream.
	 *
	 * @param cont Container of operators to add.
	 */
	template<typename Container> void addContentStreamToFront (const Container& cont);
	
	
	template<typename Container> 
	inline
	void addContentStream (const Container& cont)
	{ 
		{int THIS_FUNCTION_SHOULD_NOT_BE_USED_USE__addContentStreamToFront_OR_addContentStreamToBack__INSTEAD;}
		addContentStreamToFront (cont);
	}

	/**
	 * Add new content stream to the back. This function adds new entry in the "Contents"
	 * property of a page. The container of provided operators must form a valid
	 * contentstream. 
	 * This function should be used when supplied operators
	 * should be handled at the end e.g. should be drawn at the end which means
	 * they will appear "above" other objects.
	 *
	 * This function can be used to separate our changes from original content stream.
	 *
	 * @param cont Container of operators to add.
	 */
	template<typename Container> void addContentStreamToBack (const Container& cont);

	/**
	 * Remove content stream. 
	 * This function removes all objects from "Contents" entry which form specified contentstream.
	 *
	 * @param csnum Number of content stream to remove.
	 */
	void removeContentStream (size_t csnum);

	//
	// Page translation 
	//
public:
	/**
	 * Set transform matrix of a page. This operator will be preceding first cm
	 * operator (see pdf specification), if not found it will be the first operator.
	 *
	 * @param tm Six number representing transform matrix.
	 */
	void setTransformMatrix (double tm[6]);
	
	
	//
	// Media box 
	//
public:
	/**  
	 * Return media box of this page. 
	 *
	 * It is a required item in page dictionary (spec p.119) but can be
	 * inherited from a parent in the page tree.
	 *
	 * @return Rectangle specifying the box.
	 */
	 Rectangle getMediabox () const;

	 
	/**  
	 * Set media box of this page. 
	 * 
	 * @param rc Rectangle specifying the page metrics.
	 */
	 void setMediabox (const Rectangle& rc);

	 //
	 // Rotation
	 //
public:
	 /**
	  * Get rotation.
	  *
	  * @return Rotation degree measurment.
	  */
	 int getRotation () const;
	
	 /**
	  * Set rotation.
	  *
	  * @param rot Set rotation degree measurment.
	  */
	 void setRotation (int rot);
 
	 //
	 // Text search/find 
	 //
public:
	 /**
	  * Find all occurences of a text on this page.
	  *
	  * It uses xpdf TextOutputDevice to get the bounding box of found text.
	  *
	  * @param text Text to find.
	  * @param recs Output container of rectangles of all occurences of the text.
	  * @param params Search parameters.
	  *
	  * @return Number of occurences found.
	  */
	 template<typename RectangleContainer>
	 size_t findText (std::string text, 
					  RectangleContainer& recs, 
					  const TextSearchParams& params = TextSearchParams()) const;

	 //
	 // Helper functions
	 //
public:
	 /**
	  * Return shared pointer to the content stream.
	  * @param cc Raw ccontentstream pointer.
	  */
	  boost::shared_ptr<CContentStream> 
	  getContentStream (CContentStream* cc) 
	  {
		  for (ContentStreams::iterator it = contentstreams.begin(); it != contentstreams.end(); ++it)
			  if ((*it).get() == cc)
				  return *it;
		  
		  assert (!"Contentstream not found");
		  throw CObjInvalidOperation ();
	  }
	 
private:
	 /**
	  * Create xpdf's state and resource parameters.
	  *
	  * @param res Gfx resource parameter.
	  * @param state Gfx state parameter.
	  */
	 void createXpdfDisplayParams (boost::shared_ptr<GfxResources>& res, boost::shared_ptr<GfxState>& state);

	
private:
	/**
	 * Save changes and indicate that the object has changed by calling all
	 * observers.
	 *
	 * @param invalid If true indicate that this page has been invalidated.
	 */
	void _objectChanged (bool invalid = false);


	//
	// PdfEdit changes 
	//
public:
	/**
	 * Get n-th change.
	 * Higher change means older change.
	 */
	boost::shared_ptr<CContentStream> getChange (size_t nthchange = 0) const;

	/**
	 * Get our changes sorted.
	 * The first change is the last change. If there are no changes
	 * container is empty.
	 */
	template<typename Container>
	void getChanges (Container& cont) const;

	/**
	 * Get count of our changes.
	 */
	size_t getChangeCount () const;

	/**
	 * Draw nth change on an output device with last used display parameters.
	 *
	 * @param out Output device.
	 * @param cont Container of content streams to display
	 */
	template<typename Container>
	void displayChange (::OutputDev& out, const Container& cont) const;

	void displayChange (::OutputDev& out, const std::vector<size_t> cs) const
	{
		ContentStreams css;
		for (std::vector<size_t>::const_iterator it = cs.begin(); it != cs.end(); ++it)
		{
			if (static_cast<size_t>(*it) >= contentstreams.size())
				throw CObjInvalidOperation ();
			css.push_back (contentstreams[*it]);
		}
		displayChange (out, css);
	}

	/**
	 * Move contentstream up one level. Which means it will be repainted by less objects.
	 */
	void moveAbove (boost::shared_ptr<const CContentStream> ct);
	void moveAbove (CcPosition pos)
	{ 
		if (pos >= contentstreams.size())
			throw OutOfRange();
		moveAbove (contentstreams[pos]); 
	};

	/**
	 * Move contentstream below one level. Which means it will be repainted by more objects.
	 */
	void moveBelow (boost::shared_ptr<const CContentStream> ct);
	void moveBelow (CcPosition pos)
	{ 
		if (pos >= contentstreams.size())
			throw OutOfRange();
		moveBelow (contentstreams[pos]); 
	};

	
};


//==========================================================
// Helper functions
//==========================================================

/**
 * Check whether iproperty claimed to be a page is conforming to the pdf specification.
 * (p.118)
 *
 * @param ip IProperty.
 */
bool isPage (boost::shared_ptr<IProperty> ip);


//=====================================================================================
} // namespace pdfobjects
//=====================================================================================


#endif // _CPAGE_H
