# Multi CNN Classifier vision app

The Multi CNN Classifier allows you to perform classification in multiple drawn ROIs with different CNNs.  
This repository contains the sources of the vision app which you can build using the [IDS NXT Vision App Creator](https://en.ids-imaging.com/downloads.html).  
Feel free to make changes to the vision app to customize it to your application.

## Getting Started
### Prerequisites
#### IDS NXT Vision App Creator
To customize and build the vision app samples, we provide the [IDS NXT Vision App Creator](https://en.ids-imaging.com/downloads.html). The IDS NXT Vision App Creator is an IDE, based on Qt Creator for programming vision apps for IDS NXT cameras. The IDE includes the toolchain and the IDS NXT SDK. You can directly upload the vision apps from the development environment to your IDS NXT camera.

#### IDS NXT cockpit
To control your IDS NXT camera and to show the vision app results and images, we provide the [IDS NXT cockpit](https://en.ids-imaging.com/downloads.html).

#### IDS NXT camera setup
To run the vision app on your IDS NXT camera you will need the  
![IDS NXT API version](https://img.shields.io/badge/NXT_API-v2.4.0-008A96.svg)  
which is included in the  
![IDS NXT OS version](https://img.shields.io/badge/NXT_OS-v1.4.x-008A96.svg)  

### Documentation
#### Installing CNNs
Using the IDS NXT cockpit you can install your CNNs on the camera using the buttons in the **Files** section of the vision app.

#### Mapping of ROIs to CNNs
**Hint:** For this vision app, it is not intended to use Crawler in IDS NXT cockpit to create or modify ROIs.
Please use the configuration file provided for this purpose as described below.  

Associate the CNNs with your ROIs using a configuration file in json format. This file contains a list of your ROIs and the associated CNNs. The configuration file can be uploaded using the buttons in the **Files** section of the vision app.  
You can see an example of a possible configuration below. The entries of the configuration are as follows:
* RoiName
    * Identifier of the ROI
    * Only lower case letters, numbers and underscores are allowed.
* Cnn
    * Identifier of the CNN to be used for this ROI.
* OffsetX and OffsetY
    * Position of the ROI in relation to the image of the camera sensor. The origin is at the top left. The unit is px.
* Height and Width
    * Height and width of the ROI in px.

```
[
    {
        "RoiName": "my_roi_1",
        "Cnn": "MyCNN_1",
        "OffsetX": 50,
        "OffsetY": 120,
        "Height": 450,
        "Width": 850
    },
    {
        "RoiName": "my_roi_2",
        "Cnn": "MyCNN_2",
        "OffsetX": 450,
        "OffsetY": 100,
        "Height": 600,
        "Width": 300
    }
]
```

#### Vision app limitations
* The maximum count of supported ROIs is 20.
* Only english and german language.

## Licenses
See the [license file](./license.txt) of the vision app.

## Contact
[IDS Imaging Development Systems GmbH](https://en.ids-imaging.com/)
