
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using SlimDX.Direct3D9;
using System.Drawing;

namespace SlimHook3D
{
    public class D3D9Hooks
    {

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        public delegate int Direct3D9Device_EndSceneDelegate(IntPtr device);

        [DllImport("inject.dll")]
        public static extern void EndSceneDotNet([MarshalAs(UnmanagedType.FunctionPtr)] Direct3D9Device_EndSceneDelegate callbackPointer);

        private Direct3D9Device_EndSceneDelegate EndSceneCallback = null;

        // this is a guard to prevent this object from being garbage collected...
        private static D3D9Hooks _instance = null;

        private SlimDX.Direct3D9.Device _D3D9Device = null;
        protected byte[] _pixelData = null;

        
        // D3D9 
        private Surface _renderTarget;
        private Surface _targetNoMultiSample;
        private Format _bufferFormat;

        Rectangle _Rect = new Rectangle();

        private byte[] _map = new byte[256];
        private byte posterizationInterval = 64;


        /// <summary>
        /// Constructor
        /// </summary>
        public D3D9Hooks()    
        {
            _instance = this;
            // calculate posterization offset
            int posterizationOffset = 0;


            // calculate mapping array            
            for (int i = 0; i < 256; i++)
            {
                _map[i] = (byte)Math.Min(255, (i / posterizationInterval) * posterizationInterval + posterizationOffset);
            }
        }

        

        public int Direct3D9Device_EndScene(IntPtr devicePtr)
        {
            try
            {
                _D3D9Device = SlimDX.Direct3D9.Device.FromPointer(devicePtr);                

                if (_renderTarget == null)
                {
                    // Create offscreen surface to use as copy of render target data
                    using (SwapChain sc = _D3D9Device.GetSwapChain(0))
                    {
                        // TODO: pass in a width and height
                        _renderTarget = Surface.CreateOffscreenPlain(_D3D9Device, sc.PresentParameters.BackBufferWidth, sc.PresentParameters.BackBufferHeight, sc.PresentParameters.BackBufferFormat, Pool.SystemMemory);
                        _targetNoMultiSample = Surface.CreateRenderTarget(_D3D9Device, sc.PresentParameters.BackBufferWidth, sc.PresentParameters.BackBufferHeight, sc.PresentParameters.BackBufferFormat, MultisampleType.None, 0, false);
                        _bufferFormat = sc.PresentParameters.BackBufferFormat;
                        _Rect = new Rectangle(0, 0, sc.PresentParameters.BackBufferWidth, sc.PresentParameters.BackBufferHeight);

                    }

                }
                // Lets mess with the scene now ;)
                using (Surface backBuffer = _D3D9Device.GetBackBuffer(0, 0))
                {
                    _D3D9Device.StretchRectangle(backBuffer, _Rect, _targetNoMultiSample, _Rect, TextureFilter.Linear);                  
                    _D3D9Device.GetRenderTargetData(_targetNoMultiSample, _renderTarget);
                    
                  SlimDX.DataRectangle dataRect = _renderTarget.LockRectangle(LockFlags.None);
                  SlimDX.DataStream dataStream = dataRect.Data;
                  int height = _renderTarget.Description.Height;
                  int width = _renderTarget.Description.Width;
                  int k = height - 1;
                  int iSrcPitch = dataRect.Pitch;
                  IntPtr data = dataStream.DataPointer;
                  unsafe
                  {
                      byte* pSrcRow = (byte*)data.ToPointer();
                      for (int i = 0; i < height; i++)
                      {
                          if (k < 0) break;
                          for (int j = 0; j < width; j++)
                          {
                              // toon shade the game pixels here..
                              pSrcRow[i * iSrcPitch + j * 4] = _map[(int)pSrcRow[i * iSrcPitch + j * 4]];
                              pSrcRow[i * iSrcPitch + j * 4+1] = _map[(int)pSrcRow[i * iSrcPitch + j * 4+1]];
                              pSrcRow[i * iSrcPitch + j * 4+2] = _map[(int)pSrcRow[i * iSrcPitch + j * 4+2]];
                          }
                          
                      }
                  }

                  _renderTarget.UnlockRectangle();
                  _D3D9Device.UpdateSurface(_renderTarget, _targetNoMultiSample);
                  _D3D9Device.StretchRectangle(_targetNoMultiSample, backBuffer, TextureFilter.Linear);

                }

                
            }
            catch (Exception ex)
            {  
                // TODO: something useful
                System.IO.File.AppendAllText("C:\\tmp\\_hook.txt", ex.Message + ex.StackTrace);
            }
            return 0;
        }
        
     

        public string AddHook(string ipcSetup)
        {
            try
            {
                EndSceneCallback = new Direct3D9Device_EndSceneDelegate(Direct3D9Device_EndScene);
                EndSceneDotNet(EndSceneCallback);
            }
            catch (Exception ex)
            {
            }
            return string.Empty;
        }

    }
   
}