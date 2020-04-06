module ImgLib
  extend FFI::Library
  ffi_lib "ffi_img_lib.dylib"
  attach_function :load_image, [:pointer], :pointer
  attach_function :copy_image, [:pointer], :pointer
  attach_function :blit_image, [:pointer, :pointer, :int, :int], :void
  attach_function :save_image, [:pointer, :pointer], :int
  attach_function :free_image, [:pointer], :void
end