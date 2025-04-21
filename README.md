# Clipper2 HDK Wrapper

A Houdini HDK wrapper around [Clipper2](https://github.com/AngusJohnson/Clipper2), for curve/2D boolean operations.

---

## Features

- **Open and Closed Curve Support**: Handles open curves/lines that Houdini's Boolean SOP cannot.
- **Stable Curve Buffering**: Provides better offsetting than the PolyExpand2D SOP in some cases.
- **Windows only right now**: At some point I'll setup builds for linux, but for now its windows only, shouldnt be too hard to modify the cmake file for linux.

---

## Examples

![OTL](https://github.com/user-attachments/assets/5bc17083-6613-4e3d-b7c8-0fff6c6a00a7)
![Cutting](https://github.com/user-attachments/assets/12b5a528-0f27-402b-b5ea-c2bd7b99f39c)
![Curve cuts](https://github.com/user-attachments/assets/a17b39b0-2000-41e5-819d-f1d5a8e6058c)


---

## License

This project is licensed under the Boost License. See the [LICENSE](LICENSE) file for details.

