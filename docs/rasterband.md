## RasterBand

Note: all methods throw errors if the band has been destroyed by the dataset

#### Methods

- `toString()` : string
- `flush()` : void
- `fill(number real_value, number imag_value = 0)` : void *(throws)*
- `getStatistics(bool allow_approx, bool force)` : object *(throws)*
- `computeStatistics(bool allow_approx)` : object *(throws)*
- `setStatistics(Number min, Number max, Number mean, Number std_dev)` : void *(throws)*
- `createMaskBand(int num_samples)` : void *(throws)*
- `getMaskBand()` : [RasterBand](rasterband.md)
- `getMaskFlags()` : int
- `getMetadata(string domain = null)` : object

#### Properties

- `description` : string
- `ds` : [Dataset](dataset.md)
- `id` : int
- `size` : object
- `blockSize` : object
- `unitType` : string *(settable)*
- `dataType` : string
    + `'Byte'`
    + `'UInt16'`
    + `'Int16'`
    + `'UInt32'`
    + `'Int32'`
    + `'Float32'`
    + `'Float64'`
    + `'CInt16'`
    + `'CInt32'`
    + `'CFloat32'`
    + `'CFloat64'`
- `minimum` : Number
- `maximum` : Number
- `offset` : Number *(settable)*
- `scale` : Number *(settable)*
- `noDataValue` : Number *(settable)*
- `readOnly` : bool
- `hasArbitraryOverviews` : bool
- `categoryNames` : string[] *(settable)*
- `overviews` : [RasterBandOverviews](#overviews)
- `pixels` : [RasterBandPixels](#pixels)
- `colorInterpretation` : string
    + `"Undefined"`
    + `"Gray"`
    + `"Palette"`
    + `"Red"`
    + `"Green"`
    + `"Blue"`
    + `"Alpha"`
    + `"Hue"`
    + `"Saturation"`
    + `"Lightness"`
    + `"Cyan"`
    + `"Magenta"`
    + `"Yellow"`
    + `"Black"`

#### Overviews

- `overviews.get(int id)` : [RasterBand](rasterband.md)
- `overviews.getBySampleCount(int min_samples)` : [RasterBand](rasterband.md)
- `overviews.count()` : int
- `overviews.forEach(function iterator)` : void

#### Pixels

- `pixels.get(x, y)` : Number
- `pixels.set(x, y, value)` : void
- `pixels.read(x, y, width, height, [data], [options])` : [TypedArray](https://developer.mozilla.org/en-US/docs/Web/API/ArrayBufferView#Typed_array_subclasses) *(throws)*
     + `x` : integer
     + `y` : integer
     + `width` : integer
     + `height` : integer
     + `data` : [TypedArray](https://developer.mozilla.org/en-US/docs/Web/API/ArrayBufferView#Typed_array_subclasses) – *The array to put the data in. A new array is created if not given*
     + `options` : object
          - `buffer_width` : integer
          - `buffer_height` : integer
          - `type` : string – Can be any of the following: `'Byte'`, `'UInt16'`, `'Int16'`, `'UInt32'`, `'Int32'`, `'Float32'`, `'Float64'`, `'CInt16'`, `'CInt32'`, `'CFloat32'`, `'CFloat64'` ([GDALDataType](http://www.gdal.org/gdal_8h.html#a22e22ce0a55036a96f652765793fb7a4)).
          - `pixel_space` : integer
          - `line_space` : integer
- `pixels.write(x, y, width, height, data, [options])` : void *(throws)*
     + `x` : integer
     + `y` : integer
     + `width` : integer
     + `height` : integer
     + `data` : [TypedArray](https://developer.mozilla.org/en-US/docs/Web/API/ArrayBufferView#Typed_array_subclasses) 
     + `options` : object
          - `buffer_width` : integer
          - `buffer_height` : integer
          - `pixel_space` : integer
          - `line_space` : integer
- `pixels.readBlock(x, y, [data])` : [TypedArray](https://developer.mozilla.org/en-US/docs/Web/API/ArrayBufferView#Typed_array_subclasses) *(throws)*
     + `data` : [TypedArray](https://developer.mozilla.org/en-US/docs/Web/API/ArrayBufferView#Typed_array_subclasses) – *The array to put the data in. A new array is created if not given*
- `pixels.writeBlock(x, y, data)` : void
