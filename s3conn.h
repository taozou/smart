#ifndef INCLUDED_S3CONN_H
#define INCLUDED_S3CONN_H

//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2012, OblakSoft LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
//
// Authors: Maxim Mazeev <mazeev@hotmail.com>
//          Artem Livshits <artem.livshits@gmail.com>

//////////////////////////////////////////////////////////////////////////////
// Amazon S3 connection and related classes built on top of Amazon S3 REST API.
//////////////////////////////////////////////////////////////////////////////

///@mainpage  C++ library to access Amazon S3.  
///
/// Provides several rare features such as 
/// Amazon S3 multi-part upload, async support, HTTP proxy, HTTP tracing, 
/// supports Eucalyptus Walrus. It is tuned to utilize HTTP stack efficiently, 
/// and offers robust error handling. The library contains built-in SSL CA
/// certificates required to establish secure SSL connection to Amazon S3.

#include "asyncurl.h"

#include <exception>
#include <string>
#include <vector>

namespace webstor
{

//////////////////////////////////////////////////////////////////////////////
// Debugging support.

#ifdef DEBUG
typedef bool ( dbgShowAssertFunc )( const char *file, int line, const char *msg, bool *ignoreAll );

void
dbgSetShowAssert( dbgShowAssertFunc *callback );
#endif

//////////////////////////////////////////////////////////////////////////////
///@brief   S3 connection parameters.
///@details Pass an instance of S3Config the S3Connection constructor.
///@code
/// S3Config config = {};
/// config.accKey = ...;
/// config.secKey = ...;
/// config.isHttps = true;
/// 
/// S3Connection conn(config);
///@endcode

struct S3Config
{
    /// Access key.

    const char     *accKey;
    
    /// Secret key.

    const char     *secKey;
    
    /// An optional region-specific host endpoint for Amazon S3 or mandatory host name for Walrus.

    /// Amazon S3:
    /// Leave it NULL/empty to connect to the US Standard region.
    /// Set to "us-west-1", to connect to US West CA region.
    /// See Amazon documentation for list of available end points.
    ///
    /// Walrus or another Amazon S3-compatible storage providers:
    /// Specify a mandatory host name.

    const char     *host;

    /// Optional port name. 

    const char     *port;
   
    /// Indicates if HTTPS should be used for all requests.

    /// For Amazon S3 it's recommended to set this field to 'true'.
    /// For Walrus, it should be 'false' because Walrus doesn't support HTTPS.

    bool            isHttps;
    
    /// Indicates if storage provider is Walrus.

    bool            isWalrus;
    
    /// Optional proxy with port name: "proxy:port".

    const char     *proxy;
    
    /// Optional file name containing SSL CA certificates.

    const char     *sslCertFile;
};

//////////////////////////////////////////////////////////////////////////////
///@brief   A single bucket.
///@details A collection of S3Buckets is returned from listAllBuckets(..).

struct S3Bucket
{
                    S3Bucket() {}
                    S3Bucket( const char *_name,  const char *_creationDate ) 
                        : name( _name ), creationDate( _creationDate ) {}

    void            clear();
    std::string     name;
    std::string     creationDate;
};

inline void
S3Bucket::clear()
{
    name.clear();
    creationDate.clear();
}

//////////////////////////////////////////////////////////////////////////////
///@brief Response from 'put' and 'putPart' requests.

struct S3PutResponse 
{
                    S3PutResponse(): partNumber( 0 ) {}

    /// partNumber, set by 'putPart' request.

    int             partNumber;

    /// etag assigned to the object by Amazon S3.

    std::string     etag;
};

///@brief An abstract class to upload 'put' and 'putPart' payload.

struct S3PutRequestUploader
{
    ///@brief   A callback to upload 'put' and 'putPart' payload. 
    ///@details The method is supposed to return a number of bytes 
    /// it has written into the provided buffer @chunkBuf.
    /// If the return value is less than the chunkSize, the farther processing will be
    /// stopped.

    virtual size_t  onUpload( void *chunkBuf, size_t chunkSize ) = 0; 
};

//////////////////////////////////////////////////////////////////////////////
///@brief Response from 'get' request.

struct S3GetResponse  
{
                    S3GetResponse() : loadedContentLength( -1 ), isTruncated( false ) {}

    /// Size of the loaded content, -1 means object is not found.

    size_t          loadedContentLength;  

    /// Indicates if the buffer is small and the content has been truncated.

    bool            isTruncated;

    /// Object's etag.

    std::string     etag;
};

///@brief An abstract class to download 'get' payload.

struct S3GetResponseLoader
{
    ///@brief   A callback to fetch 'get' payload. 
    ///@details The method is supposed to return a number of bytes it has read,
    /// if the return value is less than the chunkSize, the farther processing will be
    /// stopped.

    virtual size_t  onLoad( const void *chunkData, size_t chunkSize, size_t totalSizeHint ) = 0; 
};

//////////////////////////////////////////////////////////////////////////////
///@brief Response from 'del' and 'abortMultipartUpload' requests.

struct S3DelResponse
{
};

//////////////////////////////////////////////////////////////////////////////
///@brief A single S3 object.

struct S3Object
{
                    S3Object(): size( -1 ), isDir( false ) {}

                    S3Object( const char *_key, const char *_lastModified, const char *_etag, size_t _size, bool _isDir )
                        : key( _key ), lastModified( _lastModified ), etag( _etag ), size( _size ), isDir( _isDir ) {}

    void            clear();

    /// Object key.

    std::string     key;
    
    /// Last modified time.

    std::string     lastModified;
    
    /// Object's etag.

    std::string     etag;
    
    /// Object size.

    size_t          size;
    
    /// Indicates if this is 'directory' or not.

    bool            isDir;
};

inline void
S3Object::clear()
{
    key.clear();
    lastModified.clear();
    etag.clear();
    size = -1;
    isDir = false;
}

///@brief An abstract class to enumerate S3 objects.

struct S3ObjectEnum
{
    /// A callback to enumerate S3 objects.

    virtual bool    onObject( const S3Object &object ) = 0; 
};

///@brief Response from 'listObjects' request.

struct S3ListObjectsResponse 
{
    ///@brief Indicates if this is the last page or not. If 'true', the response has been 
    /// truncated and there are more objects to read.

    bool            isTruncated;
    
    /// A marker to fetch the next page.

    std::string     nextMarker;
};

//////////////////////////////////////////////////////////////////////////////
///@brief Response from 'initiateMultipartUpload' request.

struct S3InitiateMultipartUploadResponse 
{
    /// uploadId assigned by Amazon S3.

    std::string     uploadId;
};

//////////////////////////////////////////////////////////////////////////////
///@brief Response from 'completeMultipartUpload' request.

struct S3CompleteMultipartUploadResponse  
{
    /// etag assigned to the created object.

    std::string     etag;
};

//////////////////////////////////////////////////////////////////////////////
///@brief A single multipart upload.

struct S3MultipartUpload
{
                    S3MultipartUpload(): isDir( false ) {}
  
                    S3MultipartUpload( const char *_key, const char *_uploadId, bool _isDir )
                        : key( _key ), uploadId( _uploadId ), isDir( _isDir ) {}

    void            clear();

    /// Object's key.

    std::string     key;
    
    /// uploadId.

    std::string     uploadId;
    
    /// Indicates if this is a 'directory'.

    bool            isDir;
};

inline void
S3MultipartUpload::clear()
{
    key.clear();
    uploadId.clear();
    isDir = false;
}

///@brief An abstract class to enumerate multipart uploads.

struct S3MultipartUploadEnum
{
    /// A callback to enumerate multipart uploads.

    virtual bool    onUpload( const S3MultipartUpload &upload ) = 0; 
};

///@brief Response from 'listMultipartUploads' request.

struct S3ListMultipartUploadsResponse 
{
    ///@brief Indicates if this is the last page or not. If 'true', the response has been 
    /// truncated and there are more uploads to read.

    bool            isTruncated;

    /// A key marker to read the next page.

    std::string     nextKeyMarker;

    /// A uploadId marker to read the next page.

    std::string     nextUploadIdMarker;
};

//////////////////////////////////////////////////////////////////////////////
///@brief S3 HTTP tracing type.

enum TraceInfo 
{
    S3_TRACE_INFO_TEXT = 0,
    S3_TRACE_INFO_HEADER_IN,
    S3_TRACE_INFO_HEADER_OUT,
    S3_TRACE_INFO_DATA_IN,
    S3_TRACE_INFO_DATA_OUT,
    S3_TRACE_INFO_SSL_DATA_IN,
    S3_TRACE_INFO_SSL_DATA_OUT,
    S3_TRACE_INFO_END
};

///@brief A callback to read HTTP headers and body.

typedef int ( TraceCallback ) ( void *handle, TraceInfo type, unsigned char *data, size_t size,
    void *cookie );


class S3Request;

//////////////////////////////////////////////////////////////////////////////
///@brief S3Connection to access Amazon S3 storage.
///@remark Thread-safety: the object is not thread safe.

class S3Connection
{
public:
    /// Minimum chunk size for multipart upload in MB.

    static const size_t c_multipartUploadMinPartSizeMB = 5; 

    /// Minimum chunk size for multipart upload in bytes.

    static const size_t c_multipartUploadMinPartSize = c_multipartUploadMinPartSizeMB * 1024 * 1024; 

    /// Constructs S3Connection.

                    S3Connection( const S3Config &config );
    
    /// Closes S3Connection.

                    ~S3Connection();

   ///@brief Synchronously creates a bucket.
   ///@details Creates a bucket with <b>bucketName</b> in the region matching the
   /// S3Config.host parameter 
   ///@remarks see host property for details.

   void             createBucket( const char *bucketName, bool makePublic = false );

   ///@brief Synchronously deletes a bucket.
   ///@details Deletes a bucket with <b>bucketName</b>.

   void             delBucket( const char *bucketName );

   ///@brief Synchronously lists all buckets.
   ///@details Lists all buckets and 
   /// appends bucket names into the provided output vector <b>buckets</b>.

   void             listAllBuckets( std::vector< S3Bucket > *buckets /* out */ );

   ///@brief Synchronously creates an S3 object.
   ///@details Creates S3 object identified by a <b>key</b> in a given <b>bucket</b> and 
   /// uploads <b>data</b>.

   void             put( const char *bucketName, const char *key, const void *data, size_t size,
                        bool makePublic = false, bool useSrvEncrypt = false, const char *contentType = NULL,
                        S3PutResponse *response = NULL /* out */ );

   ///@brief Synchronously creates an S3 object.
   ///@details Creates S3 object identified by a <b>key</b> in a given <b>bucket</b> and 
   /// uploads data with <b>uploader</b>. 
   /// Total size of the data being uploaded must be
   /// specified in <b>totalSize</b>.

   void             put( const char *bucketName, const char *key, S3PutRequestUploader *uploader, size_t totalSize,
                       bool makePublic = false, bool useSrvEncrypt = false, const char *contentType = NULL,
                       S3PutResponse *response = NULL /* out */ );

   ///@brief Synchronously loads an S3 object.
   ///@details Fetches content of an S3 object identified by a <b>key</b> from
   /// a given <b>bucket</b> using provided <b>loader</b> object.
   /// The <b>response</b> tells whether the object is found or not and the size of the 
   /// loaded content. If <b>loadedContentLength</b> (in S3GetResponse) is set to -1, the object is missing.
   /// If <b>isTruncated</b> is set to true, the loader stopped reading the data and
   /// only a part of the content is returned.

   void             get( const char *bucketName, const char *key, 
                        S3GetResponseLoader *loader /* in */, 
                        S3GetResponse *response = NULL /* out */ );

   ///@brief Synchronously loads an S3 object.
   ///@details Fetches content of an S3 object identified by a <b>key</b> from
   /// a given <b>bucket</b> and writes the content into the provided <b>buffer</b>.
   /// The <b>response</b> tells whether the object is found or not and the size of the 
   /// loaded content. If <b>loadedContentLength</b> (in S3GetResponse) is set to -1, 
   /// the object is missing.
   /// If <b>isTruncated</b> is set to true, the <b>buffer</b> is not large enough to hold the content
   /// and truncation happened.
   /// Consider using the other method that takes a <b>loader</b> object if you need to
   /// get hints about the total content length assuming the server provides it.

   void             get( const char *bucketName, const char *key, void *buffer, size_t size, 
                        S3GetResponse *response = NULL /* out */ );

   ///@brief Synchronously gets a page of S3 object identifiers.
   ///@details Lists up to the <b>maxKeys</b> objects (or 'directories') in a given <b>bucket</b> and 
   /// calls the provided <b>objectEnum</b> for each object name.
   /// Specify <b>prefix</b> and/or <b>marker</b> to control what objects to return and the start position.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listObjects( const char *bucketName, const char *prefix,
                        const char *marker, const char *delimiter, unsigned int maxKeys,
                        S3ObjectEnum *objectEnum,
                        S3ListObjectsResponse *response = NULL /* out */ );

   ///@brief Synchronously gets a page of S3 object identifiers.
   ///@details Lists up to the <b>maxKeys</b> objects (or 'directories') in a given <b>bucket</b> and 
   /// appends object names into the provided output vector <b>objects</b>.
   /// Specify <b>prefix</b> and/or <b>marker</b> to control what objects to return and the start position.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listObjects( const char *bucketName, const char *prefix,
                        const char *marker, const char *delimiter, unsigned int maxKeys,
                        std::vector< S3Object > *objects /* out */,
                        S3ListObjectsResponse *response = NULL /* out */ );

   ///@brief Synchronously list S3 objects.
   ///@details Lists all objects (or 'directories') in a given <b>bucket</b> and 
   /// calls the provided <b>objectEnum</b> for each object name.
   /// Specify <b>prefix</b> to control what objects to return.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listAllObjects( const char *bucketName, const char *prefix, 
                        const char *delimiter, S3ObjectEnum *objectEnum, 
                        unsigned int maxKeysInBatch = 1000 );

   ///@brief Synchronously lists S3 objects.
   ///@details Lists all objects (or 'directories') in a given <b>bucket</b> and 
   /// appends object names into the provided output vector <b>objects</b>.
   /// Specify <b>prefix</b> to control what objects to return.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listAllObjects( const char *bucketName, const char *prefix, 
                         const char *delimiter, std::vector< S3Object> *objects, 
                         unsigned int maxKeysInBatch = 1000 );

   ///@brief Synchronously deletes an S3 object.
   ///@details Deletes an object identified by a <b>key</b> from the given <b>bucket</b>.
   /// The method is no-op if the object doesn't exist.

   void             del( const char *bucketName, const char *key, S3DelResponse *response = NULL /* out */ );

   ///@brief Synchronously deletes S3 objects 
   ///@details Deletes all objects that match a <b>prefix</b> from the given <b>bucket</b>.
   /// The method is no-op if no objects exist.

   void             delAll( const char *bucketName, const char *prefix, unsigned int maxKeysInBatch = 1000 );

   // Multipart upload.

   ///@brief Synchronously initiates a multipart upload.
   ///@details Initiates a multipart upload of an object identified by a <b>key</b> into a given <b>bucket</b>.
   /// Returns an <b>uploadId</b> (in S3InitiateMultipartUploadResponse) that needs to be used 
   /// by subsequent putPart(..) and completeMultipartUpload(..) methods.

   void             initiateMultipartUpload( const char *bucketName, const char *key, 
                        bool makePublic = false, bool useSrvEncrypt = false, const char *contentType = NULL,
                        S3InitiateMultipartUploadResponse *response = NULL /* out */  );

   ///@brief Synchronously uploads a single part.
   ///@details Uploads a single part with a given <b>partNumber</b> for a multipart
   /// upload identified by <b>bucketName</b>, <b>key</b> and <b>uploadId</b>. The latter (<b>uploadId</b>) is returned by 
   /// initiateMultipartUpload(..) method.
   /// <b>size</b> of the chunk being uploaded must be at least 5MB.
   /// <b>partNumber</b> starts with 1.
   
   void             putPart( const char *bucketName, const char *key, const char *uploadId, int partNumber,
                        const void *data, size_t size, S3PutResponse *response = NULL /* out */  );
    
   ///@brief Synchronously uploads a single part.
   ///@details Uploads a single part with a given <b>partNumber</b> for a multipart
   /// upload identified by <b>bucketName</b>, <b>key</b> and <b>uploadId</b>. The latter (<b>uploadId</b>) is returned by 
   /// initiateMultipartUpload(..) method.
   /// <b>partSize</b> of the chunk being uploaded must be at least 5MB.
   /// <b>partNumber</b> starts with 1.

   void             putPart( const char *bucketName, const char *key, const char *uploadId, int partNumber,
                        S3PutRequestUploader *uploader, size_t partSize, S3PutResponse *response = NULL /* out */  );

   ///@brief Synchronously commits a multipart upload.
   ///@details Commits a multipart upload consisting of parts specified in the <b>parts</b> array
   /// (<b>parts</b> is the pointer to the first element and <b>size</b> is the number of elements in the array).
   /// The multipart upload is identified by <b>bucketName</b>, <b>key</b> and <b>uploadId</b>. 
   /// The latter (<b>uploadId</b>) is returned by initiateMultipartUpload(..) method.

   void             completeMultipartUpload( const char *bucketName, const char *key, const char *uploadId, 
                        const S3PutResponse *parts, size_t size, 
                        S3CompleteMultipartUploadResponse *response = NULL /* out */ );

   ///@brief Synchronously aborts a multipart upload.
   ///@details Aborts a multipart upload identified by <b>bucketName</b>, <b>key</b> and <b>uploadId</b>. 
   /// The latter (<b>uploadId</b>) is returned by initiateMultipartUpload(..) method.

   void             abortMultipartUpload( const char *bucketName, const char *key, const char *uploadId, 
                        S3DelResponse *response = NULL /* out */ );

   ///@brief Synchronously aborts all multipart uploads.
   ///@details Aborts all multipart uploads that match <b>prefix</b> from a given <b>bucketName</b>.
   
   void             abortAllMultipartUploads( const char *bucketName, const char *prefix,
                        unsigned int maxUploadsInBatch = 1000 );

   ///@brief Synchronously gets a page of multipart uploads. 
   ///@details Lists up to the <b>maxUploads</b> multipart uploads 
   /// (or 'directories' where they are started) in a given <b>bucket</b> and 
   /// calls the provided <b>uploadEnum</b> for each upload id.
   /// Specify <b>prefix</b> and/or <b>keyMarker</b> with <b>uploadIdMarker</b> to control what uploads
   /// to return and the start position.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listMultipartUploads( const char *bucketName, const char *prefix,
                        const char *keyMarker, const char *uploadIdMarker, const char *delimiter, 
                        unsigned int maxUploads,
                        S3MultipartUploadEnum *uploadEnum,
                        S3ListMultipartUploadsResponse *response = NULL /* out */ );

   ///@brief Synchronously gets a page of multipart uploads.
   ///@details Lists up to the <b>maxUploads</b> multipart uploads 
   /// (or 'directories' where they are started) in a given <b>bucket</b> and 
   /// populates the provided <b>uploads</b> vector.
   /// Specify <b>prefix</b> and/or <b>keyMarker</b> with <b>uploadIdMarker</b> to control what uploads
   /// to return and the start position.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listMultipartUploads( const char *bucketName, const char *prefix,
                        const char *keyMarker, const char *uploadIdMarker, const char *delimiter, 
                        unsigned int maxUploads,
                        std::vector< S3MultipartUpload > *uploads /* out */,
                        S3ListMultipartUploadsResponse *response = NULL /* out */ );

   ///@brief Synchronously lists multipart uploads.
   ///@details Lists all uploads (or 'directories' where they are started) 
   /// in a given <b>bucket</b> and  calls the provided <b>uploadEnum</b> object.
   /// Specify <b>prefix</b> to control what uploads to return.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listAllMultipartUploads( const char *bucketName, const char *prefix, 
                       const char *delimiter, S3MultipartUploadEnum *uploadEnum, 
                       unsigned int maxUploadsInBatch = 1000 );

   ///@brief Synchronously lists multipart uploads.
   ///@details Lists all uploads (or 'directories' where they are started) 
   /// in a given <b>bucket</b> and appends them to the provided <b>uploads</b> vector.
   /// Specify <b>prefix</b> to control what uploads to return.
   /// To list directories, pass the <b>delimiter</b> parameter which specifies what
   /// character (e.g. '/') is used as a directory separator.

   void             listAllMultipartUploads( const char *bucketName, const char *prefix, 
                       const char *delimiter, std::vector< S3MultipartUpload > *uploads, 
                       unsigned int maxUploadsInBatch = 1000 );

   // Async support.
   
   ///@brief Starts asynchronous <b>put</b> request.
   ///@details Asynchronously creates S3 object identified by a <b>key</b> in a given <b>bucket</b> and 
   /// uploads <b>data</b>.
   /// Both <b>asyncMan</b> and the <b>data</b> buffer that holds data being uploaded
   /// must be available till the completePut(..) or cancelAsync(..) methods are called.
   /// Only one async operation can be started with a given S3Connection instance.
   /// If you need to start another one, call complete or cancel first. Or use
   /// multiple S3Connections.
   
   void             pendPut( AsyncMan *asyncMan, const char *bucketName, const char *key, 
                        const void *data, size_t size,
                        bool makePublic = false, bool useSrvEncrypt = false);

   ///@brief Waits and completes the asynchronous <b>put</b> request.
   ///@details Completes the started asynchronous put operation. The method blocks till the operation finishes.
   /// After the method returns, the caller can start another sync or async operation.

   void             completePut( S3PutResponse *response = NULL /* out */ );

   ///@brief Starts asynchronous <b>get</b> request.
   ///@details Asynchronously fetches content of an S3 object identified by a <b>key</b> from
   /// a given <b>bucket</b> and writes the content into the provided <b>buffer</b>.
   /// Both <b>asyncMan</b> and the <b>buffer</b> that holds data being downloaded
   /// must be available till the completeGet(..) or cancelAsync(..) methods are called.
   /// Only one async operation can be started with a given S3Connection instance.
   /// If you need to start another one, call complete or cancel first. Or use
   /// multiple S3Connections.

   void             pendGet( AsyncMan *asyncMan, const char *bucketName, const char *key, 
                        void *buffer, size_t size, size_t offset = -1);

   ///@brief Waits and completes the asynchronous <b>get</b> request.
   ///@details Completes the started asynchronous get operation. The method blocks till the operation finishes.
   /// After the method returns, the caller can start another sync or async operation.
   /// The <b>response</b> tells whether the object is found or not and the size of the 
   /// loaded content. 
   /// If <b>loadedContentLength</b> (in S3GetResponse) is set to -1, 
   /// the object is missing.
   /// If <b>isTruncated</b> is set to true, the <b>buffer</b> is not large enough to hold the content
   /// and truncation happened.
   void             completeGet( S3GetResponse *response = NULL /* out */ );

   ///@brief Starts asynchronous <b>del</b> request.
   ///@details Asynchronously deletes an S3 object identified by a <b>key</b> from
   /// a given <b>bucket</b>.
   /// The <b>asyncMan</b> must be available till the completeDel(..) or cancelAsync(..) 
   /// methods are called.

   void             pendDel( AsyncMan *asyncMan, const char *bucketName, const char *key );

   ///@brief Waits and completes the asynchronous <b>del</b> request.
   ///@details Completes the started asynchronous del operation. The method blocks till the operation finishes.
   /// After the method returns, the caller can start another sync or async operation.

   void             completeDel( S3DelResponse *response = NULL /* out */ );

   /// Returns true if an async operation is in progress.

   bool             isAsyncPending(); // nofail   

   /// Returns true if an async operation is completed.

   bool             isAsyncCompleted(); // nofail

   /// Cancel any pending async operations.

   void             cancelAsync(); // nofail

   /// Maximum number of S3Connections waitAny(..) supports.

   enum { c_maxWaitAny = 128 };

   ///@brief Waits for any S3Connection to complete async operation, returns -1 if timeout.
   ///@details <b>startFrom</b> specifies connection index to start the check from.
   /// The caller can change this index in round-robin fashion to ensure fairness.
   /// <b>count</b> specifies number of connections in <b>cons</b> array. This number should be
   /// less or equal to c_maxWaitAny.

   static int       waitAny( S3Connection **cons, size_t count, size_t startFrom = 0,
                            long timeout = -1 /* infinite */ );

   /// Sets timeouts, in milliseconds.

   void             setTimeout( long timeout ); 

   /// Sets connect timeouts, in milliseconds.

   void             setConnectTimeout( long connectTime ); 

   /// Enables HTTP tracing.

   void             enableTracing( TraceCallback *traceCallback ) { m_traceCallback = traceCallback; }

private:
                    S3Connection( const S3Connection & );  // forbidden
     S3Connection & operator=( const S3Connection & );  // forbidden

    void            prepare( S3Request *request, const char *bucketName, const char *key,
                        const char *contentType = NULL,
                        bool makePublic = false, bool useSrvEncrypt = false, size_t low = 0, size_t high = 0);

    void            init( S3Request *request, const char *bucketName, const char *key, 
                        const char *keySuffix = NULL, const char *contentType = NULL, 
                        bool makePublic = false, bool useSrvEncrypt = false, size_t low = 0, size_t high = 0);

    void            put( S3Request *request, const char *bucketName, const char *key, 
                        const char *uploadId, int partNumber, 
                        bool makePublic, bool useSrvEncrypt, const char *contentType,
                        S3PutResponse *response );

    void            del( const char *bucketName, const char *key, const char *keySuffix, 
                        S3DelResponse *response );

    std::string     m_accKey;
    std::string     m_secKey;
    std::string     m_baseUrl;
    std::string     m_region;
    bool            m_isWalrus;
    bool            m_isHttps;
    std::string     m_proxy;
    std::string     m_sslCertFile;

    char            m_errorBuffer[ 256 ];
    TraceCallback * m_traceCallback;

    internal::AsyncCurl m_curl;

    // Async support.

    S3Request *     m_asyncRequest;

    // Timeouts.

    long            m_timeout;          // in milliseconds
    long            m_connectTimeout;   // in milliseconds
};

}  // namespace webstor

#endif // !INCLUDED_S3CONN_H
