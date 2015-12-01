# Main Page {#mainpage}

CLIF is a file format and corresponding library based on hdf5, to simplify light field handling and implement basic processing.

# Limitations

CLIF is a work in progress. The API is not yet fixed and the following restrictions are currently in place:
- Datastore and Attribute APIs: clif::Mat was introduced a short while ago to handle arbitrary multi-dimensional data - most of the API is still based on OpenCV's cv::Mat and will be transitioned step by step
- Inconsistent handling of clif::Datastore: at the moment there are two api's, an image based (cv::Mat) one which allows sub-array access and a clif::Mat one which only handles complete read/write of the whole store. Currently those APIs cannot be mixed directly, although after writing to disk a new store may use different API. This has to be fixed soon!

# Documentation Overview

- [The clif File Format](\ref file_format)
- [Tools and File Handling](\ref tools)
- [Introduction to CLIF](\ref library)
- [Library Documentation (clif namespace)](@ref clif)
