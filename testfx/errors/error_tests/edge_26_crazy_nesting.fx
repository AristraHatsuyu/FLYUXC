// 疯狂嵌套和混合
if (true) {
    for (i := 0; i < 10; i++) {
        while (true) {
            try {
                break
            } catch (e) {
                continue
            }
        }
    }
}
